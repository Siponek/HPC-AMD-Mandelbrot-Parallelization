import matplotlib.pyplot as plt

# Data
threads = list(range(1, 25))

execution_times = [
    12.75, 6.38, 7.86, 5.25, 5.20, 3.93, 3.80, 3.19, 3.46, 2.86, 3.18, 2.22, 2.19, 2.76, 2.07, 2.57, 2.48, 2.29, 2.24, 2.25, 2.04, 1.89, 1.75, 1.85
]

base_time = execution_times[0]
speedups = [base_time / time for time in execution_times]

ideal_speedup = threads

# Plotting
plt.figure(figsize=(10, 6))
plt.plot(threads, speedups, marker='o', linestyle='-', label='Speedup')
plt.plot(threads, ideal_speedup, linestyle='--', label='Ideal Speedup (Linear)')
best_speedup = max(speedups)
best_speedup_index = speedups.index(best_speedup)
plt.plot(threads[best_speedup_index], best_speedup, marker='o', markersize=10, markerfacecolor='none', markeredgewidth=2, label=f'Best Speedup ({best_speedup:.2f})')
plt.title('Speedup vs. Number of Threads')
plt.xlabel('Number of Threads')
plt.ylabel('Speedup')
plt.grid(True)
plt.xticks(threads)
plt.yticks(threads)
#plt.yticks(range(0, int(max(speedups)) + 1))
plt.tight_layout()
plt.legend()
plt.show()
# plt.savefig(f'./report/images/speedup_mpi.png')