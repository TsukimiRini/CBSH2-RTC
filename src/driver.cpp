/* Copyright (C) Jiaoyang Li
* Unauthorized copying of this file, via any medium is strictly prohibited
* Confidential
* Written by Jiaoyang Li <jiaoyanl@usc.edu>, March 2021
*/

/*driver.cpp
* Solve a MAPF instance on 2D grids.
*/
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include "CBS.h"
#include <stdlib.h>
#include "munkres-cpp/munkres.h"
#include <algorithm>
#include <chrono>
#include <random>

/* Main function */
int main(int argc, char **argv)
{
	namespace po = boost::program_options;
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()("help", "produce help message")

		// params for the input instance and experiment settings
		("map,m", po::value<string>()->required(), "input file for map")("agents,a", po::value<string>()->required(), "input file for agents")("output,o", po::value<string>(), "output file for schedule")("outputPaths", po::value<string>(), "output file for paths")("outputSteps", po::value<string>(), "output file for each step")("agentNum,k", po::value<int>()->default_value(0), "number of agents")("cutoffTime,t", po::value<double>()->default_value(60), "cutoff time (seconds)")("nodeLimit", po::value<int>()->default_value(MAX_NODES), "node limit")("screen,s", po::value<int>()->default_value(1), "screen option (0: none; 1: results; 2:all)")("seed,d", po::value<int>()->default_value(0), "random seed")("stats", po::value<bool>()->default_value(false), "write to files some statistics")("agentIdx", po::value<string>()->default_value(""), "customize the indices of the agents (e.g., \"0,1\")")

		// params for instance generators
		("rows", po::value<int>()->default_value(0), "number of rows")("cols", po::value<int>()->default_value(0), "number of columns")("obs", po::value<int>()->default_value(0), "number of obstacles")("warehouseWidth", po::value<int>()->default_value(0), "width of working stations on both sides, for generating instances")

		// params for CBS
		("heuristics", po::value<string>()->default_value("WDG"), "heuristics for the high-level search (Zero, CG,DG, WDG)")("prioritizingConflicts", po::value<bool>()->default_value(true), "conflict priortization. If true, conflictSelection is used as a tie-breaking rule.")("bypass", po::value<bool>()->default_value(true), "Bypass1")("disjointSplitting", po::value<bool>()->default_value(false), "disjoint splitting")("rectangleReasoning", po::value<string>()->default_value("GR"), "rectangle reasoning strategy (None, R, RM, GR, Disjoint)")("corridorReasoning", po::value<string>()->default_value("GC"), " corridor reasoning strategy (None, C, PC, STC, GC, Disjoint")("mutexReasoning", po::value<bool>()->default_value(false), "Using mutex reasoning")("targetReasoning", po::value<bool>()->default_value(true), "Using target reasoning")("restart", po::value<int>()->default_value(1), "number of restart times (at least 1)")("sipp", po::value<bool>()->default_value(false), "using sipp as the single agent solver")

		// params for pick-up
		("pickUpGenre", po::value<string>()->default_value("none"), "genre of pick up (none, OneByOne, IncrByTime, Disappear)")("maxStep", po::value<int>(), "maximum step")("goalDistri", po::value<string>(), "the way of goals distribution (random, best)");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help"))
	{
		cout << desc << endl;
		return 1;
	}

	po::notify(vm);
	/////////////////////////////////////////////////////////////////////////
	/// check the correctness and consistence of params
	//////////////////////////////////////////////////////////////////////
	heuristics_type h;
	if (vm["heuristics"].as<string>() == "Zero")
		h = heuristics_type::ZERO;
	else if (vm["heuristics"].as<string>() == "CG")
		h = heuristics_type::CG;
	else if (vm["heuristics"].as<string>() == "DG")
		h = heuristics_type::DG;
	else if (vm["heuristics"].as<string>() == "WDG")
		h = heuristics_type::WDG;
	else
	{
		cout << "WRONG heuristics strategy!" << endl;
		return -1;
	}

	rectangle_strategy r;
	if (vm["rectangleReasoning"].as<string>() == "None")
		r = rectangle_strategy::NR; // no rectangle reasoning
	else if (vm["rectangleReasoning"].as<string>() == "R")
		r = rectangle_strategy::R; // rectangle reasoning for entire paths
	else if (vm["rectangleReasoning"].as<string>() == "RM")
		r = rectangle_strategy::RM; // rectangle reasoning for path segments
	else if (vm["rectangleReasoning"].as<string>() == "GR")
		r = rectangle_strategy::GR; // generalized rectangle reasoning
	else if (vm["rectangleReasoning"].as<string>() == "Disjoint")
		r = rectangle_strategy::DR; // disjoint rectangle reasoning
	else
	{
		cout << "WRONG rectangle reasoning strategy!" << endl;
		return -1;
	}

	corridor_strategy c;
	if (vm["corridorReasoning"].as<string>() == "None")
		c = corridor_strategy::NC; // no corridor reasoning
	else if (vm["corridorReasoning"].as<string>() == "C")
		c = corridor_strategy::C; // corridor reasoning
	else if (vm["corridorReasoning"].as<string>() == "PC")
		c = corridor_strategy::PC; // corridor + pseudo-corridor reasoning
	else if (vm["corridorReasoning"].as<string>() == "STC")
		c = corridor_strategy::STC; // corridor with start-target reasoning
	else if (vm["corridorReasoning"].as<string>() == "GC")
		c = corridor_strategy::GC; // generalized corridor reasoning = corridor with start-target + pseudo-corridor
	else if (vm["corridorReasoning"].as<string>() == "Disjoint")
		c = corridor_strategy::DC; // disjoint corridor reasoning
	else
	{
		cout << "WRONG corridor reasoning strategy!" << endl;
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////
	/// load the instance
	//////////////////////////////////////////////////////////////////////
	Instance instance(vm["map"].as<string>(), vm["agents"].as<string>(),
					  vm["agentNum"].as<int>(), vm["agentIdx"].as<string>(),
					  vm["rows"].as<int>(), vm["cols"].as<int>(), vm["obs"].as<int>(), vm["warehouseWidth"].as<int>());

	srand(vm["seed"].as<int>());

	int runs = vm["restart"].as<int>();

	//////////////////////////////////////////////////////////////////////
	/// initialize the solver
	//////////////////////////////////////////////////////////////////////
	CBS cbs(instance, vm["sipp"].as<bool>(), vm["screen"].as<int>());
	cbs.setPrioritizeConflicts(vm["prioritizingConflicts"].as<bool>());
	cbs.setDisjointSplitting(vm["disjointSplitting"].as<bool>());
	cbs.setBypass(vm["bypass"].as<bool>());
	cbs.setRectangleReasoning(r);
	cbs.setCorridorReasoning(c);
	cbs.setHeuristicType(h);
	cbs.setTargetReasoning(vm["targetReasoning"].as<bool>());
	cbs.setMutexReasoning(vm["mutexReasoning"].as<bool>());
	cbs.setSavingStats(vm["stats"].as<bool>());
	cbs.setNodeLimit(vm["nodeLimit"].as<int>());

	//////////////////////////////////////////////////////////////////////
	/// run
	//////////////////////////////////////////////////////////////////////
	double runtime = 0;
	int min_f_val = 0;
	if (vm["pickUpGenre"].as<string>() != "none")
	{
		int max_runs = vm["maxStep"].as<int>();
		runs = max_runs;
	}

	for (int i = 0; i < runs; i++)
	{
		cbs.clear();
		cbs.solve(vm["cutoffTime"].as<double>(), min_f_val, MAX_COST, vm["agents"].as<string>());
		runtime += cbs.runtime;
		if (vm["pickUpGenre"].as<string>() != "none")
		{
			if (!cbs.solution_found)
			{
				cout << "ERROR" << endl;
				return 0;
			}
			vector<int> reach_goal_agent, reached_goal;
			vector<int> new_start(cbs.num_of_agents);
			vector<int> goals = instance.getGoals();
			int min_path_len = 1;
			int reached = 0;
			for (int i = 0;; i++)
			{
				bool goToNext = false;
				for (int j = 0; j < cbs.num_of_agents; j++)
				{
					int point = (*cbs.paths[j])[i].location;
					vector<int>::iterator itr = std::find(goals.begin(), goals.end(), point);
					if (itr != goals.end())
					{
						goToNext = true;
						reached++;
						reach_goal_agent.push_back(std::distance(goals.begin(), itr));
						reached_goal.push_back(point);
					}
				}
				if (goToNext)
				{
					min_path_len = i + 1;
					break;
				}
			}

			// for (int i = 0; i < cbs.num_of_agents; i++)
			// {
			// 	int path_size = cbs.paths[i]->size();
			// 	if (path_size < min_path_len){
			// 		reached = 1;
			// 		min_path_len = path_size;
			// 	}else if(path_size == min_path_len){
			// 		reached++;
			// 	}
			// }

			vector<bool> grid_occupied(instance.num_of_cols * instance.num_of_rows);
			for (int i = 0; i < grid_occupied.size(); i++)
				grid_occupied[i] = false;
			for (int i = 0; i < cbs.num_of_agents; i++)
			{
				int point = (*cbs.paths[i])[min_path_len - 1].location;
				new_start[i] = point;
				grid_occupied[point] = true;
			}
			instance.changeStartLocations(new_start);

			int residual = grid_occupied.size() - cbs.num_of_agents;
			for (int idx : reach_goal_agent)
			{
				int rand_res = rand() % residual;
				int i = 0;
				for (int block_i = 0; block_i < grid_occupied.size(); block_i++)
				{
					if (grid_occupied[block_i])
						continue;
					if (i == rand_res)
					{
						goals[idx] = block_i;
						break;
					}
					i++;
				}
				residual--;
			}

			system_clock::time_point start = system_clock::now();

			vector<int> new_goals(cbs.num_of_agents);
			if (vm["goalDistri"].as<string>() == "random")
			{
				unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
				std::shuffle(goals.begin(), goals.end(), std::default_random_engine(seed));
				new_goals.assign(goals.begin(), goals.end());
			}
			else if (vm["goalDistri"].as<string>() == "best")
			{
				munkres_cpp::Matrix<int> data(cbs.num_of_agents, goals.size());
				for (int i = 0; i < cbs.num_of_agents; i++)
				{
					for (int j = 0; j < goals.size(); j++)
					{
						int s_row = instance.getRowCoordinate(new_start[i]), s_col = instance.getColCoordinate(new_start[i]);
						int e_row = instance.getRowCoordinate(goals[j]), e_col = instance.getColCoordinate(goals[j]);
						data(i, j) = abs(e_row - s_row) + abs(e_col - s_col);
					}
				}

				munkres_cpp::Munkres<int> solver(data);

				for (int i = 0; i < cbs.num_of_agents; i++)
				{
					for (int j = 0; j < goals.size(); j++)
					{
						if (data(i, j) == 0)
						{
							new_goals[i] = goals[j];
							break;
						}
					}
					cout << endl;
				}
			}

			runtime += std::chrono::duration<double, std::deca>(system_clock::now() - start).count();

			instance.changeGoalLocations(new_goals);

			cbs.resetInstance(instance, vm["sipp"].as<bool>(), vm["screen"].as<int>());
		}
		else
		{
			if (cbs.solution_found)
				break;
			min_f_val = (int)cbs.min_f_val;
			cbs.randomRoot = true;
		}
	}
	cbs.runtime = runtime;

	//////////////////////////////////////////////////////////////////////
	/// write results to files
	//////////////////////////////////////////////////////////////////////
	if (vm.count("output"))
		cbs.saveResults(vm["output"].as<string>(), vm["agents"].as<string>() + ":" + vm["agentIdx"].as<string>());
	// cbs.saveCT(vm["output"].as<string>() + ".tree"); // for debug
	if (vm["stats"].as<bool>())
	{
		cbs.saveStats(vm["output"].as<string>(), vm["agents"].as<string>() + ":" + vm["agentIdx"].as<string>());
	}
	if (vm.count("outputPaths"))
		cbs.savePaths(vm["outputPaths"].as<string>());
	if (vm.count("outputSteps"))
		cbs.saveSteps(vm["outputSteps"].as<string>(), vm["agents"].as<string>() + ":" + vm["agentIdx"].as<string>());
	cbs.clearSearchEngines();
	return 0;
}