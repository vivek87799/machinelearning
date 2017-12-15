import csv

data_instance = list()
data_ins = list()
ulabel = set()
labelcounter = dict()
nodes = dict()


class Node:
    label_name = ""
    attribute = dict()
    label_count = int

    def __init__(self, label_name, label_count):
        self.label_name = label_name
        self.label_count = label_count
        self.attribute = dict()


# opening the data file
with open("../cardaten/car.data", 'rb') as dataset_file:
    data_set = csv.reader(dataset_file, delimiter=',')
    for ds in data_set:
        data_instance.append(ds)
        ulabel.add(ds[len(ds) - 1])
total_instance = len(data_instance)
c = len(ulabel)
label_pos = len(data_instance[0]) - 1

# Classifier
for di in data_instance:
    labelcounter[di[label_pos]] = labelcounter.get(di[label_pos], 0) + 1

for label in labelcounter:
    nodes[label] = Node(label, labelcounter[label])
    print label

for di in data_instance:
    cnode = nodes[di[label_pos]]
    for i in range(len(di) - 1):
        try:
            _feature = cnode.attribute[i]
        except:
            _feature = {}
        _feature[di[i]] = _feature.get(di[i], 0) + 1
        cnode.attribute[i] = _feature

print nodes['acc'].attribute[0]['high']


# Predictor
def predictLabel(nodes, instance=None):
    for n in nodes:
        i = 0
        val = 0.0
        val = (nodes[n].label_count / float(total_instance))
        for d in instance:
            try:
                val = val + (nodes[n].attribute[i][d] / float(nodes[n].label_count))
            except:
                val = 0
                break
            i += 1
        print n
        print val


test_instance = ['vhigh' ,'low' ,'3' ,'4' ,'big' ,'high']
test_instance = ['low' ,'high' ,'3' ,'more' ,'big' ,'high']
predictLabel(nodes, test_instance)
print("classified")
