import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import sys
import os

# ── figure style ──────────────────────────────────────────────────────────────
# single-column width for two-column latex paper is ~3.5 inches
# pdf.fonttype 42 embeds fonts as TrueType, required for most latex workflows
COL_W = 3.5
COL_H = 2.6

plt.rcParams.update({
    'font.family':      'serif',
    'font.size':        8,
    'axes.titlesize':   8,
    'axes.labelsize':   8,
    'xtick.labelsize':  7,
    'ytick.labelsize':  7,
    'legend.fontsize':  7,
    'figure.dpi':       150,
    'pdf.fonttype':     42,
    'ps.fonttype':      42,
})

# ── load data ─────────────────────────────────────────────────────────────────
CSV = 'scaling_results.csv'
if not os.path.exists(CSV):
    sys.exit(f"error: {CSV} not found")

cols = ['spinners', 'mode', 'matrix_time_s', 'matrix_gflops',
        'bench_iters', 'bench_elapsed_s', 'bench_iters_per_s', 'bench_cpu_ms']

df = pd.read_csv(CSV, header=0, names=cols, skiprows=1)

# coerce numeric columns; missing fields (,,) become NaN
for c in ['spinners', 'matrix_time_s', 'matrix_gflops', 'bench_iters']:
    df[c] = pd.to_numeric(df[c], errors='coerce')

alone  = df[df['mode'] == 'alone'].iloc[0]
coop   = df[df['mode'] == 'coop'].sort_values('spinners').reset_index(drop=True)
nocoop = df[df['mode'] == 'nocoop'].sort_values('spinners').reset_index(drop=True)

alone_gflops = alone['matrix_time_s']   # just to verify parse
alone_gflops = alone['matrix_gflops']
alone_time   = alone['matrix_time_s']

x = coop['spinners'].values  # shared x-axis for all per-spinner plots

def save(fig, name):
    fig.tight_layout()
    fig.savefig(name, bbox_inches='tight')
    print(f"saved {name}")

# ── graph 1: matrix gflops vs spinner count ───────────────────────────────────
# primary result: shows how well cooperative scheduling protects matrix throughput
fig, ax = plt.subplots(figsize=(COL_W, COL_H))

ax.axhline(alone_gflops, color='black', linestyle='--', linewidth=1,
           label=f'alone baseline ({alone_gflops:.2f})')
ax.plot(x, coop['matrix_gflops'],   'o-', color='steelblue',  label='cooperative')
ax.plot(x, nocoop['matrix_gflops'], 's-', color='firebrick',  label='non-cooperative')

ax.set_xlabel('spinner thread count')
ax.set_ylabel('GFLOPS')
ax.set_title('Matrix Throughput vs. Spinner Contention')
ax.set_xticks(x)
ax.legend()
ax.grid(True, linestyle=':', linewidth=0.5)
save(fig, 'graph1_gflops.pdf')

# ── graph 2: matrix time overhead (%) vs spinner count ────────────────────────
# normalised cost: shows relative slowdown rather than absolute time
# makes the coop vs nocoop divergence clearer than raw seconds
fig, ax = plt.subplots(figsize=(COL_W, COL_H))

coop_overhead   = (coop['matrix_time_s']   - alone_time) / alone_time * 100
nocoop_overhead = (nocoop['matrix_time_s'] - alone_time) / alone_time * 100

ax.plot(x, coop_overhead,   'o-', color='steelblue', label='cooperative')
ax.plot(x, nocoop_overhead, 's-', color='firebrick', label='non-cooperative')
ax.axhline(0, color='black', linestyle='--', linewidth=1, label='alone baseline')

ax.set_xlabel('spinner thread count')
ax.set_ylabel('time overhead vs. alone (%)')
ax.set_title('Matrix Time Overhead vs. Spinner Contention')
ax.set_xticks(x)
ax.legend()
ax.grid(True, linestyle=':', linewidth=0.5)
save(fig, 'graph2_overhead.pdf')

# ── graph 3: benchmark iterations vs spinner count ────────────────────────────
# confirms the scheduler is actually deprioritising the spinner:
# lower iterations in coop mode = fewer cpu timeslices received
fig, ax = plt.subplots(figsize=(COL_W, COL_H))

ax.plot(x, coop['bench_iters'],   'o-', color='steelblue', label='cooperative')
ax.plot(x, nocoop['bench_iters'], 's-', color='firebrick', label='non-cooperative')

ax.set_xlabel('spinner thread count')
ax.set_ylabel('total iterations')
ax.set_title('Spinner Iterations vs. Thread Count')
ax.set_xticks(x)
ax.yaxis.set_major_formatter(ticker.FuncFormatter(
    lambda v, _: f'{v/1e9:.1f}B' if v >= 1e9 else f'{v/1e6:.0f}M'))
ax.legend()
ax.grid(True, linestyle=':', linewidth=0.5)
save(fig, 'graph3_iterations.pdf')

# ── graph 4: iteration suppression ratio (nocoop / coop) ─────────────────────
# derived metric: how many more iterations the non-cooperative spinner completes
# relative to the cooperative one at the same thread count.
# a ratio of N means the cooperative spinner consumed ~1/N the cpu of nocoop.
# rising ratio with thread count shows the penalty scales with contention.
fig, ax = plt.subplots(figsize=(COL_W, COL_H))

ratio = nocoop['bench_iters'].values / coop['bench_iters'].values
ax.bar(x, ratio, color='steelblue', width=0.6, edgecolor='black', linewidth=0.5)
ax.axhline(1, color='firebrick', linestyle='--', linewidth=1, label='no suppression (ratio=1)')

ax.set_xlabel('spinner thread count')
ax.set_ylabel('nocoop iters / coop iters')
ax.set_title('CPU Suppression Ratio of Cooperative Scheduling')
ax.set_xticks(x)
ax.legend()
ax.grid(True, axis='y', linestyle=':', linewidth=0.5)
save(fig, 'graph4_suppression_ratio.pdf')

plt.close('all')
