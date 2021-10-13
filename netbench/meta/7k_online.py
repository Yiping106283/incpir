import matplotlib.pyplot as plt
import numpy as np


fig, ax1 = plt.subplots()


font = { 'family': 'Times New Roman',
'weight' : 'normal',
'size'   : 24,
}

ax1.set_xlabel('throughput (queries/sec)', font)
ax1.set_ylabel('mean latency (msec)', font)

# metadata: throughput for fixed 0.1 sec
# figure: throughput for 1 sec

x_dpf = [15, 38, 32, 48, 57, 61, 43]
y_dpf = [44, 43, 69, 94, 139, 143, 364]
x_dpf = [ele*10 for ele in x_dpf]

x_ours = [85, 262, 313, 351, 389, 362] 
y_ours = [41, 41, 41, 41, 41, 290]
x_ours = [ele*10 for ele in x_ours]

#x_ours_refresh = [85, 263, 321, 397, 423]
#y_ours_refresh = [41, 41, 41, 41, 407]
#x_ours_refresh = [ele*10 for ele in x_ours_refresh]

x_ock = [47, 86, 112, 141, 187, 271, 219, 233]
y_ock = [41, 41, 41, 41, 41, 43, 189, 280]
x_ock = [ele*10 for ele in x_ock]

my_line_width = 2

ax1.plot(x_dpf, y_dpf, color='black',
          linestyle='dashed', linewidth=my_line_width,
          marker='X', markerfacecolor='black', markersize=6,
          label='DPF-PIR')
ax1.plot(x_ock, y_ock, color='purple',
        linestyle='dashed', linewidth=my_line_width,
        marker='X', markerfacecolor='purple', markersize=6,
        label='CK')
ax1.plot(x_ours, y_ours, color='orchid',
          linestyle='dashed', linewidth=my_line_width,
          marker='X', markerfacecolor='orchid', markersize=6,
          label='incremental CK')

ax1.spines['bottom'].set_linewidth(2)
ax1.spines['left'].set_linewidth(2)

ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)
ax1.legend(ncol=3,loc='upper center',bbox_to_anchor=(0.5, 1.25), framealpha=0.0, 
           prop={'family': 'Times New Roman', 'weight': 'normal', 'size': 18}, columnspacing=0.7,  borderaxespad=0.)
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)
plt.xlim(xmin=0, xmax=4500)
plt.ylim(ymin=0, ymax=250)


plt.tick_params(labelsize=18)
labels = ax1.get_xticklabels() + ax1.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]


fig.tight_layout()
fig.savefig("Figure-10a.pdf")

