import csv

data_instance = list()
labelcounter = dict()
nodes = dict()
filepath = "../cardaten/car.data"
k = 1

class Node:
    label_name = ""
    attribute = dict()
    label_count = int

    def __init__(self, label_name, label_count):
        self.label_name = label_name
        self.label_count = label_count
        self.attribute = dict()


# opening the data file
with open(filepath, 'rb') as dataset_file:
    data_set = csv.reader(dataset_file, delimiter=',')
    for ds in data_set:
        data_instance.append(ds)
        labelcounter[ds[len(ds) - 1]] = labelcounter.get(ds[len(ds) - 1], 0) + 1
total_instance = len(data_instance)
label_pos = len(data_instance[0]) - 1


# Classifier
def bayesClassifier():
    for label in labelcounter:
        nodes[label] = Node(label, labelcounter[label])

    for di in data_instance:
        cnode = nodes[di[label_pos]]
        for i in range(len(di) - 1):
            try:
                _feature = cnode.attribute[i]
            except:
                _feature = {}
            _feature[di[i]] = _feature.get(di[i], 0) + 1
            cnode.attribute[i] = _feature


# Predictor
def predictLabel(nodes, instance=None):
    # TODO use laplace smoothing
    maxval = 0.0
    for n in nodes:
        i = 0
        val = 0.0
        maxval = 0.0
        val = ((nodes[n].label_count + k)/ ((float(total_instance)) + (k * len(nodes))))
        for d in instance:
            try:
                val = val * ((nodes[n].attribute[i][d] + k) / ((float(nodes[n].label_count)) + (k * len(nodes[n].attribute[i]))))
            except:
                val = val * ((k) / (float(nodes[n].label_count)) + (k * len(nodes[n].attribute[i])))
                break
            i += 1
        if val > maxval:
            maxval = val
            print maxval
            label = nodes[n].label_name
            print label

    return label


# Calling the classifier
bayesClassifier()
# med ,low ,2 ,4 ,med ,high
test_instance = ['vhigh' ,'low' ,'3' ,'4' ,'big' ,'high']
test_instance = ['med' ,'low' ,'2' ,'4' ,'med' ,'high']
label = predictLabel(nodes, test_instance)
print label
print("classified")



