times = {
    "Python": 10.8641939163208,        # Python版本测得时间
    "C/C++": 0.055000,          # 版本2测得时间
    "调整循环顺序": 0.033000,    # 版本3测得时间
    "编译优化": 0.003000,       # 版本4测得时间
    "循环展开": 0.005000,        # 版本5测得时间
    "Intel MKL": 0.002000        # 版本6测得时间
}

N = 256
peak_GFLOPS_1 = 51.2
peak_GFLOPS_2 = 204.8 

def compute_metrics(times, N):
    results = {}
    baseline = times["Python"]
    keys = list(times.keys())

    for idx, version in enumerate(keys):
        t = times[version]
        relative_speedup = '-' if idx == 0 else round(times[keys[idx-1]]/t, 2)
        absolute_speedup = round(baseline/t, 2)
        GFLOPS = round((2*N**3)/(t*1e9), 2)
        peak_percent = round((GFLOPS/peak_GFLOPS_1)*100, 2) if idx != 5 else round((GFLOPS/peak_GFLOPS_2)*100, 2)

        results[version] = {
            "运行时间(sec.)": t,
            "相对加速比": relative_speedup,
            "绝对加速比": absolute_speedup,
            "GFLOPS": GFLOPS,
            "峰值性能百分比(%)": peak_percent
        }
    return results

results = compute_metrics(times, N)

print(f"{'版本':<15}{'运行时间(s)':<15}{'相对加速比':<10}{'绝对加速比':<10}{'GFLOPS':<10}{'峰值性能百分比(%)':<15}")
for ver, metric in results.items():
    print(f"{ver:<15}{metric['运行时间(sec.)']:<17}{metric['相对加速比']:<12}{metric['绝对加速比']:<12}{metric['GFLOPS']:<10}{metric['峰值性能百分比(%)']:<15}")
