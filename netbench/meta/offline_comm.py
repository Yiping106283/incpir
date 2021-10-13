# 7k nodes

import numpy as np
import matplotlib.pyplot as plt

data = np.array([[21, 2700, 21, 2700], 
                 [27, 2700, 47, 2700]])   

length = len(data)
x_labels = ['CK', 'incremental CK']

font = {'family' : 'Times New Roman',
'weight' : 'normal',
'size'   : 24,
}

# Set plot parameters

fig, ax = plt.subplots()
width = 0.2 # width of bar
x = np.arange(length)

ax.bar(x, data[:,0], width, color='darkviolet', label='Prep: query')
ax.bar(x + width, data[:,1], width, color='mediumorchid', label='Prep: response')
ax.bar(x + 2*width, data[:,2], width, color='orange', label='IncPrep: query')
ax.bar(x + 3*width, data[:,3], width, color='gold', label='IncPrep: response')


ax.set_ylabel('communication (KB)', font)
#ax.set_xticklabels(x_labels, fontsize=30)

ax.set_ylim(1, 9000)
ax.set_yscale('log')
ax.set_xticks(x + width + width/2)
ax.set_xticklabels(x_labels)

ax.spines['bottom'].set_linewidth(2)
ax.spines['left'].set_linewidth(2)

#ynew = 2
#ax.axhline(ynew, ls='--', linewidth=0.8, color='black', label='single item size')
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)
#ax.yaxis.grid(True, which='major', 'y', ls='--', lw=.5, c='k', alpha=.3)

ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)

ax.legend(ncol=2, framealpha=0, loc='upper center', bbox_to_anchor=(0.55, 1.25) ,
  prop={'family':'Times New Roman', 'size':18}, columnspacing=0.5) 

plt.tick_params(labelsize=22)
labels = ax.get_xticklabels() + ax.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]

fig.tight_layout()
fig.savefig("Figure-11b.pdf")

