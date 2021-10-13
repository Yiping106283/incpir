import matplotlib.pyplot as plt
import numpy as np

fig, ax1 = plt.subplots()


font = { 'family': 'Times New Roman',
'weight' : 'normal',
'size'   : 30,
}

ax1.set_xlabel('throughput (queries/min)', font)
ax1.set_ylabel('mean latency (sec)', font)


# metadata: throughput for fixed 10 sec
# figures: throughput for 60 sec 


x_baseline_1 = [10, 15, 20, 21, 20]
x_baseline_1 = [ele*6 for ele in x_baseline_1] 
y_baseline_1 = [0.7067, 0.6849, 0.7907, 1.5672, 3.139]

x_baseline_5 = [5, 15, 20, 21]
x_baseline_5 = [ele*6 for ele in x_baseline_5]
y_baseline_5 = [0.6369, 0.6731, 0.6531, 2.949 ]

x_ours_1 = [30, 40, 50, 51, 48]
x_ours_1 = [ele*6 for ele in x_ours_1]
y_ours_1 = [0.4263, 0.3879, 0.4722, 1.4939, 2.689]

x_ours_5 = [20, 30, 40, 40, 40]
x_ours_5 = [ele*6 for ele in x_ours_5]
y_ours_5 = [0.5054, 0.4771, 0.4816, 1.6787, 2.936]

my_line_width = 2

ax1.plot(x_baseline_1, y_baseline_1, color='blue',
          linestyle='dashed', linewidth=my_line_width,
          marker='s', markerfacecolor='blue', markersize=6,
          label='Prep (1%)')
ax1.plot(x_baseline_5, y_baseline_5, color='blue',
          linestyle='dashed', linewidth=my_line_width,
          marker='o', markerfacecolor='blue', markersize=6,
          label='Prep (5%)')

ax1.plot(x_ours_1, y_ours_1, color='red',
          linestyle='dashed', linewidth=my_line_width,
          marker='s', markerfacecolor='red', markersize=6,
          label='IncPrep (1%)')
ax1.plot(x_ours_5, y_ours_5, color='red',
          linestyle='dashed', linewidth=my_line_width,
          marker='o', markerfacecolor='red', markersize=6,
          label='IncPrep (5%)')


plt.xlim(xmin=0, xmax = 350)
plt.ylim(ymin=0,ymax = 3.5)

ax1.spines['bottom'].set_linewidth(2)
ax1.spines['left'].set_linewidth(2)

ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)

plt.rcParams['font.family'] = 'serif'

ax1.legend(ncol=2, loc='upper center', bbox_to_anchor=(0.5, 1.2), 
  framealpha=0.0, prop={'family':'Times New Roman', 'weight': 'normal', 'size': 20}, 
  columnspacing=1, borderaxespad=0.)
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)


plt.tick_params(labelsize=22)
labels = ax1.get_xticklabels() + ax1.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]


fig.tight_layout()
fig.savefig("Figure-10b.pdf")

