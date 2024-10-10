## Source code 2

## What is modified SimBlock

**Modified SimBlock** is a simulator that accelerates the original SimBlock by removing the Gossip protocol. In SimBlock (SimBlock URL: [https://github.com/dec-love/simblock_for_delta_measurement](https://github.com/dec-love/simblock_for_delta_measurement)), the Gossip protocol is simulated first to obtain the worst propagation time between each node. This worst propagation time is then set as the propagation delay between nodes, enabling high-speed simulation.

## How to Use the Simulator

1. First, set the `max_delta` obtained from SimBlock in the `delay` variable of the `multiNetwork_for_cg_same_prop.cpp` file.
2. Execute the following command to run the simulation for 1 million blocks:

   ```bash
   g++ -std=c++11 multiNetwork_for_cg_same_prop.cpp
   ./a.out
   ```

3. Based on the results saved in `simulation_results_T600.csv`, visualize the stale block rate using the following Python script:

   ```bash
   python3 stalerate_vs_s_graph.py
   ```
