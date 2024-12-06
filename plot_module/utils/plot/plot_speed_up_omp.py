import matplotlib.pyplot as plt

# Data
threads = list(range(1, 25))
'''
execution_times_no_sched = [
    13.10, 6.55, 8.07, 5.39, 5.35, 4.05, 3.89, 3.26, 3.07,
    2.81, 2.89, 2.53, 2.35, 2.27, 2.06, 1.89, 1.86, 1.83,
    1.70, 1.78, 1.67, 1.57, 1.50, 1.51
]
'''
execution_times_dynamic = [
    13.08, 6.56, 4.37, 3.28, 2.62, 2.19, 1.96, 1.77, 1.62, 1.47, 1.37, 1.30, 1.24, 1.16, 1.14, 1.13, 1.06, 1.00, 1.03, 1.10, 1.01, 0.97, 0.95, 0.96
]
execution_times_guided = [
    13.09, 6.55, 4.45, 3.29, 2.63, 2.22, 1.98, 1.79, 1.65, 1.56, 1.44, 1.37, 1.28, 1.19, 1.17, 1.09, 1.09, 1.03, 1.05, 0.98, 0.99, 1.02, 0.98, 0.99
]
execution_times_static = [
    13.10, 6.55, 8.09, 5.39, 5.35, 4.04, 3.90, 3.25, 3.25, 2.70, 2.52, 2.35, 2.41, 2.30, 1.99, 1.98, 1.88, 1.86, 1.89, 1.70, 1.57, 1.63, 1.61, 1.56
]

schedulers = [execution_times_dynamic, execution_times_guided, execution_times_static]
schedulers_names = ["Dynamic", "Guided", "Static"]

for scheduler_name, scheduler in zip(schedulers_names, schedulers):
    base_time = scheduler[0]
    speedups = [base_time / time for time in scheduler]

    ideal_speedup = threads

    # Plotting
    plt.figure(figsize=(10, 6))
    plt.plot(threads, speedups, marker='o', linestyle='-', label='Speedup')
    plt.plot(threads, ideal_speedup, linestyle='--', label='Ideal Speedup (Linear)')
    best_speedup = max(speedups)
    best_speedup_index = speedups.index(best_speedup)
    plt.plot(threads[best_speedup_index], best_speedup, marker='o', markersize=10, markerfacecolor='none', markeredgewidth=2, label=f'Best Speedup ({best_speedup:.2f})')
    plt.title(f'Speedup vs. Number of Threads - {scheduler_name} scheduler')
    plt.xlabel('Number of Threads')
    plt.ylabel('Speedup')
    plt.grid(True)
    plt.xticks(threads)
    plt.yticks(threads)
    #plt.yticks(range(0, int(max(speedups)) + 1))
    plt.tight_layout()
    plt.legend()
    plt.savefig(f'./report/images/speedup_omp_{scheduler_name}.png')