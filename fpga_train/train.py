from re import T
import numpy as np
from half_llc_env import half_llc_env
from ray.rllib.agents.ppo import PPOTrainer
import matplotlib.pyplot as plt
import os

trainer = PPOTrainer(
    config={
        # Env class to use (here: our gym.Env sub-class from above).
        "env": half_llc_env,
        # Parallelize environment rollouts.
        "num_workers": 1,
        "num_gpus": 0,
        "model":{"fcnet_hiddens": [64, 64, 64]},
        "gamma" : 0.99,
        "train_batch_size": 1024,
        "sgd_minibatch_size": 1024,
        "lr":0.000005,
        "num_sgd_iter": 16,
        "rollout_fragment_length": 1024,
        "env_config":{
            "netlist": [
                'Vs Vs 0 400',
                'S1 Vs Vin .1',
                'S2 Vin 0 .1',
                'Cr Vin Vcl 24n',
                'Lr Vcl Vpp 70u',
                'L1 Vpp 0 280u',
                'L2 Vd1 0 968n',
                'L3 0 Vd2 968n',
                'D1 Vd1 Vtout 0',
                'D2 Vd2 Vtout 0',
                'Rds Vtout Vout .001',
                'CL Vout 0 1000u',
                'RL Vout 0 0.48',
                'K1 L1 L2 1',
                'K2 L2 L3 1',
                'K3 L1 L3 1'
            ],
            "Cr_list": [
                560e-12,
                680e-12,
                820e-12,
                1e-9,
                1.2e-9,
                1.5e-9,
                1.8e-9,
                2.2e-9,
                2.7e-9,
                3.3e-9,
                3.9e-9,
                4.7e-9,
                5.6e-9,
                6.8e-9,
                8.2e-9,
                10e-9,
                12e-9,
                15e-9,
                18e-9,
                22e-9,
                27e-9,
                33e-9,
                39e-9,
                47e-9,
                56e-9,
                68e-9,
                82e-9,
                100e-9,
                150e-9,
                220e-9,
                470e-9,
                1e-6
            ],
            "Lr_LN": 157e-9
        },
    })

# Train for n iterations and report results (mean episode rewards).
# Since we have to move at least 19 times in the env to reach the goal and
# each move gives us -0.1 reward (except the last move at the end: +1.0),
# we can expect to reach an optimal episode reward of -0.1*18 + 1.0 = -0.8
result = []
for i in range(200):
    results = trainer.train()
    result.append(results)
    print(f"Iter: {i}; avg. reward={results['episode_reward_mean']}")

total_reward = []
for i in range(200):
    total_reward.append(result[i]['episode_reward_mean'])
    print(f"Iter: {i}; avg. reward={result[i]['episode_reward_mean']}")

plt.plot(np.array(total_reward))
plt.show()

np.savetxt('train_Curve',np.array(total_reward))
