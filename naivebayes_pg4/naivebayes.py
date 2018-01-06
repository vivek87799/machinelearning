import csv
import random

K = 0.01
filepath = "../cardaten/car.data"
data_instance = list()
labelcounter = dict()
nodes = dict()

# Each node is a class
class Node:
    label_name = ""
    attribute = dict()
    label_count = int

    def __init__(self, label_name, label_count):
        self.label_name = label_name
        self.label_count = label_count
        self.attribute = dict()


# Split the data into test data, training data
def splitData (data_instance):
    random.shuffle(data_instance)
    train_data = data_instance[:int(len(data_instance) * (2 / 3.0))]
    test_data = data_instance[int(len(data_instance) * (2 / 3.0)):]
    return train_data, test_data


# Calculate error rate
def calcErrorRate():
    return


# Classifier
# Creates the likelihood table
def bayesClassifier(train_data):
    for label in labelcounter:
        nodes[label] = Node(label, labelcounter[label])

    for di in train_data:
        cnode = nodes[di[label_pos]]
        for i in range(len(di) - 1):
            try:
                _feature = cnode.attribute[i]
            except:
                _feature = {}
            _feature[di[i]] = _feature.get(di[i], 0) + 1
            cnode.attribute[i] = _feature


# Predictor
# Takes an instance and returns the expected class for that instance
def predictLabel(nodes, test_data, instance=None):
    e = 0
    for index, td in enumerate(test_data):
        maxval = 0.0
        for n in nodes:
            i = 0
            val = 0.0
            val = ((nodes[n].label_count + K) / ((float(total_instance)) + (K * len(nodes))))
            for d in td:
                # error is caught when we try to calculate for the missing feature
                try:
                    val = val * ((nodes[n].attribute[i][d] + K) / ((float(nodes[n].label_count)) + (K * len(nodes[n].
                                                                                                            attribute[
                                                                                                                i]))))
                except:
                    val = val * (K / (float(nodes[n].label_count)) + (K * len(nodes[n].attribute[i])))
                i += 1
                if i >= len(td)-1:
                    break
            # Checks if the probability is max then gives the label for that max
            if val > maxval:
                maxval = val
                label = nodes[n].label_name
        if td[len(td)-1] != label:
            e += 1
        td.append(label)
    print "error"
    print e
    er = (float(e)/len(test_data)) * 100
    return label, er


# Error rate calculator
# er is the error rate
def errorRateCalculator():
    random.shuffle(train_data)
    sample_data = train_data[:100]
    label, er = predictLabel(nodes, sample_data)
    return sample_data


# Creating a confusion matrix
def confusionMatrix():
    a = len(labelcounter) + 1
    cf_matrix = [[int(0) for x in range(a)] for y in range(a)]
    i = 0
    for l in labelcounter:
        i = i + 1
        cf_matrix[0][i] = l
        cf_matrix[i][0] = l
    cflist = labelcounter.keys()
    for cf in sample_data:
        cf_matrix[cflist.index(cf[len(cf) - 2])+1][cflist.index(cf[len(cf) - 1])+1] = cf_matrix[cflist.index(cf[len(cf) - 2])+1][cflist.index(cf[len(cf) - 1])+1] + 1
    return cf_matrix


# opening the data file
with open(filepath, 'r') as dataset_file:
    data_set = csv.reader(dataset_file, delimiter=',')
    for ds in data_set:
        data_instance.append(ds)
        labelcounter[ds[len(ds) - 1]] = labelcounter.get(ds[len(ds) - 1], 0) + 1
total_instance = len(data_instance)
label_pos = len(data_instance[0]) - 1

train_data, test_data = splitData(data_instance)

# Calling the classifier
bayesClassifier(train_data)
test_instance = ['vhigh', 'low', '3', '4', 'big', 'high']
test_instance = ['med' ,'low' ,'2' ,'4' ,'med' ,'high']
# Send the node along with the nodes obtained from classifier and the test instance
label, er = predictLabel(nodes, test_data)
# To calculate the error rate
sample_data = errorRateCalculator()
cf_matrix = confusionMatrix()
print er
print("classified")



