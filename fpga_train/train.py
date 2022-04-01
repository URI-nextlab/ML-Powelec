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
        "gamma" : 1,
        "train_batch_size": 1024,
        "sgd_minibatch_size": 1024,
        "lr":0.00005,
        "num_sgd_iter": 16,
        "rollout_fragment_length": 1024,

    })

# Train for n iterations and report results (mean episode rewards).
# Since we have to move at least 19 times in the env to reach the goal and
# each move gives us -0.1 reward (except the last move at the end: +1.0),
# we can expect to reach an optimal episode reward of -0.1*18 + 1.0 = -0.8
result = []
for i in range(60):
    results = trainer.train()
    result.append(results)
    print(f"Iter: {i}; avg. reward={results['episode_reward_mean']}")

total_reward = []
for i in range(60):
    total_reward.append(result[i]['episode_reward_mean'])
    print(f"Iter: {i}; avg. reward={result[i]['episode_reward_mean']}")

plt.plot(np.array(total_reward))
plt.show()
