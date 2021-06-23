import os
import numpy

map_dir = "../data/map"
scen_dir = "../data/scen"
init_dir = "../init_envs"
outputs_dir = "../outputs"

num_agents = 2
while num_agents < 1024:
    num_agents *= 2
    for size in [10, 20, 40, 80, 160]:
        if not os.path.exists(os.path.join(map_dir, str(size) + ".map")):
            f = open(os.path.join(map_dir, str(size) + ".map"), "w")
            f.write(str(size) + "," + str(size))
            f.close()
        if not os.path.exists(os.path.join(outputs_dir, "paths", str(size))):
            os.makedirs(os.path.join(outputs_dir, "paths", str(size)))
        if not os.path.exists(os.path.join(scen_dir, str(size))):
            os.makedirs(os.path.join(scen_dir, str(size)))
        if size == 10 and num_agents > 32:
            continue
        if size == 20 and num_agents > 128:
            continue
        if size == 40 and num_agents > 512:
            continue
        for density in [0]:
            for iter in range(100):
                init_fname = str(num_agents) + "_agents_" + str(
                    size) + "_size_0_density_id_" + str(iter) + ".npy"
                [s_arr, e_arr] = numpy.load(os.path.join(init_dir, init_fname))
                with open(
                        os.path.join(scen_dir, str(size),
                                     str(init_fname[:-4]) + ".scen"),
                        "w") as f:
                    f.write(str(num_agents) + "\n")
                    for i in range(size):
                        for j in range(size):
                            if s_arr[i][j] > 0:
                                f.write("s,{},{},{}\n".format(
                                    s_arr[i][j] - 1, i, j))
                            if e_arr[i][j] > 0:
                                f.write("e,{},{},{}\n".format(
                                    e_arr[i][j] - 1, i, j))
