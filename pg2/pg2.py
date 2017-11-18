import math
import csv

data_instance = list()
data_ins = list()
calc = list()
attribute_tree_parent = list()
attribute_tree_child = list()
ulabel = set()
ulabel_count = dict()
variable_count = dict()
ulabel_dict = dict()
dict_labelcount = dict()
features_count = dict()
features_label_count = dict()
ent = float(0)
t = int
nodes = list()
attribute_gains = list()

class Nodes:
    data_instance = list()
    data_instance_length = None
    child = dict()
    entropy = -1.0
    gain = None
    pvalue = None
    attribute = -1
    ulabel = None
    parent = None
    leaflabel = list()
    childname = None
    def __init__(self, parent,  data_instance,  attribute):
        # TODO create a parents list node and remove the unused nodes
        self.parent = parent
        self.data_instance = data_instance
        self.data_instance_length = len(data_instance)
        self.attribute = attribute
        self.entropy = -1.0
        self.child = dict()
        self.ulabel_count = dict()
    # Get the data instances to be used by the child nodes and the length of the nodes
    def getDataInstance(self, data_instance, value):
        new_data_instance = list()
        for di in data_instance:
            # TODO using "in" list treats high and vhigh the same
            # below is the fix if di[self.attribute] in value:
            if value == di[self.attribute] :
                new_data_instance.append(di)
        return new_data_instance;

    def getChildNodes(self,pvalue):
        for value in pvalue:
            di_temp = list()
            di_temp = self.getDataInstance(self.data_instance[:], value)
            # TODO
            if len(di_temp) > 0:
                self.child[value] = Nodes(self, di_temp[:], -1)
                self.child[value].ulabel = self.getULables(di_temp[:], value, self.child[value].ulabel_count)
                self.child[value].childname = value
                # variable_count[di[self.parent.attribute]] = variable_count.get(di[self.parent.attribute], 0) + 1

        return

    # Get the unique labels for the data instance and variable value
    def getULables(self, data_instance, value=-1, ulabel_count=None):
        ulabel = set()
        if value == -1:
            for ds in data_instance:
                self.ulabel_count[ds[len(ds) - 1]] = self.ulabel_count.get(ds[len(ds) - 1], 0) + 1
                ulabel.add(ds[len(ds) - 1])
            return ulabel;
        else:
            ulabel.clear()
            for ds in data_instance:
                if ds[self.attribute] in value:
                    ulabel_count[ds[len(ds) - 1]] = ulabel_count.get(ds[len(ds) - 1], 0) + 1
                    ulabel.add(ds[len(ds) - 1])
            return ulabel;


    # Calculate the entropy
    def calcEntropy(self, fi, ulabel, data_instance, total_instance):
        global features_count
        c = len(ulabel)
        feature_ent = 0.0
        features_count.clear()

        if c == 1:
            self.entropy = 0
            return 0

        # calculating individual features count
        # features_count.keys() contains the variables for that feature
        # features_label_count contains the count of the label for a particular variable

        for di in data_instance:
            features_count[di[fi]] = features_count.get(di[fi], 0) + 1
        for f in features_count.keys():
            features_label_count.clear()
            for di in data_instance:
                if f == di[fi]:
                    features_label_count[di[6]] = features_label_count.get(di[6], 0) + 1
            for fl in features_label_count:
                pi = (float(features_label_count[fl]) / total_instance)
                feature_ent = feature_ent - (pi * math.log(pi, c))
        self.entropy = feature_ent
        return feature_ent


    # Calculate the gain
    def calcGain(self, fi, ulabel, data_instance, total_instance):
        global features_count
        c = len(ulabel)
        feature_ent = 0.0
        gain = 0.0
        svds = 0.0
        features_count.clear()

        if c == 1:
            self.leaflabel = list(ulabel)
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
                    # TODO change index to the index of the label to be generic
                    features_label_count[di[6]] = features_label_count.get(di[6], 0) + 1
            pi = 0.0
            feature_ent = 0.0
            for fl in features_label_count:
                pi = (float(features_label_count[fl]) / features_count[f])
                feature_ent = feature_ent - (pi * math.log(pi, c))
            svds = float(features_count[f]) / total_instance
            gain = (svds * feature_ent) + gain
        gain = self.entropy - gain
        return gain


# To get the node child and dita instances for that node
def getMaxGain(feature_entropy):
    max_value = max(feature_entropy)
    if max_value == 0:
        return -1
    max_index = feature_entropy.index(max_value)
    return max_index;

# Get the feature variables from the list
def getFeatureVariables(matrix, i):
    return [row[i] for row in matrix]


# opening the data file
with open("cardaten/car.data", 'rb') as dataset_file:
    data_set = csv.reader(dataset_file, delimiter=',')
    for ds in data_set:
        data_instance.append(ds)
        ulabel.add(ds[len(ds)-1])
total_instance = len(data_instance)

feature_entropy = list()

#Creating the tree
def createDecisionTree(rnode=None):
    attribute_gains[:] = []


    # To get the parents for the current node which are to be omitted to calc the gain

    parents = list()
    par = rnode.parent
    while par != None:
        if par.attribute != -1:
            parents.append(par.attribute)
            par = par.parent
            continue
        par = par.parent

    # Calculating the entropy
    rnode.entropy = rnode.calcEntropy(len(rnode.data_instance[0]) - 1, rnode.ulabel, rnode.data_instance,
                                      rnode.data_instance_length)

    # Calculate the gain of all the child attributes
    # TODO before calculating gain check if there is only one classifier
    if len(rnode.ulabel) == 1:
        rnode.leaflabel = rnode.ulabel
        return
    for fi in range(len(data_instance[0]) - 1):
        if fi in parents:
            attribute_gains.append(0)
            continue
        else:
            gain_value = rnode.calcGain(fi, rnode.ulabel, rnode.data_instance, rnode.data_instance_length)
            if gain_value == 1:
                attribute_gains.append(1)
                rnode.attribute = fi
                rnode.leaflabel = rnode.ulabel
                return
            else:
                attribute_gains.append(gain_value)

    # Calculate the max gain to obtain the parent node
    attribute_index = getMaxGain(attribute_gains)
    if attribute_index == -1:
        rnode.leaflabel = list(rnode.ulabel)
        par = rnode
        while par != None:
            par = par.parent
        return
    attribute_values = list(set(getFeatureVariables(rnode.data_instance, attribute_index)))

    # Setting the current node attribute i.e naming the current node
    rnode.attribute = attribute_index
    print("entropy")
    print (rnode.entropy)
    print ("parent")
    print (parents)
    print ("current node")
    print (rnode.attribute)
    print ("node name")
    print (rnode.childname)
    print ("node label count")
    print(rnode.ulabel_count)
    print ("leaf label")
    print(rnode.leaflabel)
    # Create child nodes
    rnode.getChildNodes(attribute_values[:])
    if len(rnode.child.keys()) == 0:
        return
    for childnode in rnode.child.keys():
        createDecisionTree(rnode.child[childnode])



# Sample call
# Now the node is created and being populated with required data to calc the entropy and gain

rnode = Nodes(None, data_instance[:], -1)
rnode.ulabel = rnode.getULables(data_instance)
createDecisionTree(rnode)


################################################################
print("labeled")





