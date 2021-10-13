import numpy as np
import matplotlib.pyplot as plt

data = np.array([[0.1*2, 2.6*2], [0.5*2, 2.6*2], 
                 [0.4*2, 2.6*2]])   

length = len(data)
x_labels = ['DPF-PIR', 'CK', 'incremental CK']

font = {'family' : 'Times New Roman',
'weight' : 'normal',
'size'   : 24,
}

# Set plot parameters
fig, ax = plt.subplots()
width = 0.2 # width of bar
x = np.arange(length)

ax.bar(x, data[:,0], width, color='#000080', label='Online: query')
ax.bar(x + width, data[:,1], width, color='#0F52BA', label='Online: response')


ax.set_ylabel('communication (KB)', font)
#ax.set_xticklabels(x_labels, fontsize=30)

ax.set_ylim(0, 6)
ax.set_xticks(x + width/2)
ax.set_xticklabels(x_labels)
#ax.set_xlabel('Scenario')
#ax.set_title('Title')

ax.spines['bottom'].set_linewidth(2)
ax.spines['left'].set_linewidth(2)

#ynew = 2
#ax.axhline(ynew, ls='--', linewidth=0.8, color='black', label='single item size')
plt.grid(True, 'major', 'y', ls='--', lw=.5, c='k', alpha=.3)
#ax.yaxis.grid(True, which='major', 'y', ls='--', lw=.5, c='k', alpha=.3)
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)

ax.legend(ncol=2, framealpha=0, loc='upper center', bbox_to_anchor=(0.5, 1.2) ,
  prop={'family':'Times New Roman', 'size':18})

plt.tick_params(labelsize=24)
labels = ax.get_xticklabels() + ax.get_yticklabels()
# print labels
[label.set_fontname('Times New Roman') for label in labels]

fig.tight_layout()
fig.savefig("Figure-11a.pdf")

