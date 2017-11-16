import math
import csv
import logging

data_instance = list()
data_ins = list()
calc = list()
ulabel = set()
ulabel_dict = dict()
dict_labelcount = dict()
features_count = dict()
features_label_count = dict()
ent = float(0)
t = int


# Get the feature variables from the list
def getFeatureVariables(matrix, i):
    return [row[i] for row in matrix]


# Calculating overall entropy
def calcEntropy(ulabel, data_instance, total_instance):
    global ent
    c = len(ulabel)
    if c == 1:
        ent = 0.0
        return 1
    # reset the global entropy to 0
    ent = 0.0
    dict_labelcount = dict.fromkeys(ulabel,0)
    for di in data_instance:
       dict_labelcount[di[6]] = dict_labelcount.get(di[6], 0) + 1

    for ul in ulabel:
        pi = float(dict_labelcount.get(ul)) / total_instance

        # TODO
        # throws error when pi is 1 check as log base 1 value 1 is undefined
        if pi != 0:
            ent = ent - (pi * math.log(pi,c))

    print(ent)
    return;

# Calculating feature overall entropy
def calcFeatureEntropy (fi,ulabel,data_instance,total_instance):
    global features_count
    c = len(ulabel)
    feature_ent = 0.0
    gain = 0.0
    svds = 0.0
    features_count.clear()

    if c == 1:
        return 1

    # calculating individual features count
    # features_count.keys() contains the variables for that feature
    # features_label_count contains the count of the label for a particular variable

    for di in data_instance:
        features_count[di[fi]] = features_count.get(di[fi], 0) + 1
    gain = 0.0
    for f in features_count.keys():
        features_label_count.clear()
        for di in data_instance:
            if f == di[fi]:
                features_label_count[di[6]] = features_label_count.get(di[6], 0) + 1
        pi = 0.0
        feature_ent = 0.0
        for fl in features_label_count:
            pi = (float(features_label_count[fl])/features_count[f])
            feature_ent=feature_ent - (pi * math.log(pi,c))
        svds = float(features_count[f])/total_instance
        gain = (svds * feature_ent) + gain
    gain = ent - gain
    return gain


# opening the data file
with open("cardaten/car.data", 'rb') as dataset_file:
    data_set = csv.reader(dataset_file, delimiter=',')
    for ds in data_set:
        data_instance.append(ds)
        ulabel.add(ds[len(ds)-1])
total_instance = len(data_instance)

feature_entropy = list()
calcEntropy(ulabel, data_instance, total_instance)
print(len(data_instance[0]))
for i in range(len(data_instance[0]) - 1):
    print(i)
    feature_entropy.append(calcFeatureEntropy(i, ulabel, data_instance, total_instance))
    print(calcFeatureEntropy(i, ulabel, data_instance, total_instance))

print(feature_entropy)
max_value = max(feature_entropy)
max_index = feature_entropy.index(max_value)
print(max_index)
print(max_value)
print(set(getFeatureVariables(data_instance, max_index)))
li = list()
li = list(set(getFeatureVariables(data_instance, max_index)))

for l in li:
    del data_ins[:]
    ulabel.clear()
    for di in data_instance:
        if di[max_index] in l:
            data_ins.append(di)
            ulabel.add(di[len(di) - 1])
            t = len(data_ins)
    calcEntropy(ulabel, data_ins, t)
    print(len(data_instance[0]) - 1)
    for i in range(len(data_instance[0]) - 1):
        if i != max_index:
            print(calcFeatureEntropy(i, ulabel, data_ins, t))
    print(l)
    print(t)







