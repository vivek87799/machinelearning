import math
import csv
import xml.etree.ElementTree as ET

'''
A simple and a generic decision tree algorithm generator
'''
data_instance = list()
data_ins = list()
calc = list()
ulabel = set()
ulabel_count = dict()
features_count = dict()
features_label_count = dict()
ent = float(0)
nodes = list()
attribute_gains = list()
c = int
len_instance = int

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
    leaflabel = None
    nodevalue = None
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
                self.child[value].nodevalue = value
        return

    # Get the unique labels for the data instance and variable value
    def getULables(self, data_instance, value=-1, ulabel_count=None):
        ulabel = set()
        # To calculate the entropy of the root
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
        global c
        feature_ent = 0.0
        features_count.clear()

        for di in data_instance:
            features_count[di[fi]] = features_count.get(di[fi], 0) + 1

        feature_ent1 = 0.0
        for f in features_count.keys():
            pi = (float(features_count[f]) / total_instance)
            plogc = pi * math.log(pi, c)
            feature_ent = feature_ent - (plogc)
        self.entropy = feature_ent
        return feature_ent

    # Calculate the gain
    def calcGain(self, fi, ulabel, data_instance, total_instance):
        global features_count
        feature_ent = 0.0
        gain = 0.0
        svds = 0.0
        features_count.clear()
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
                    features_label_count[di[len_instance - 1]] = features_label_count.get(di[len_instance - 1], 0) + 1
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
c = len(ulabel)
len_instance = len(data_instance[0])
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

    for fi in range(len(data_instance[0]) - 1):
        if fi in parents:
            attribute_gains.append(0)
            continue
        else:
            gain_value = rnode.calcGain(fi, rnode.ulabel, rnode.data_instance, rnode.data_instance_length)
            attribute_gains.append(gain_value)

    # Calculate the max gain to obtain the parent node
    attribute_index = getMaxGain(attribute_gains)
    if attribute_index == -1:
        rnode.leaflabel = ''.join(rnode.ulabel)
        par = rnode
        while par != None:
            par = par.parent
        return
    attribute_values = list(set(getFeatureVariables(rnode.data_instance, attribute_index)))

    # Setting the current node attribute i.e naming the current node
    rnode.attribute = attribute_index

    # Create child nodes
    rnode.getChildNodes(attribute_values[:])
    if len(rnode.child.keys()) == 0:
        return
    for childnode in rnode.child.keys():
        createDecisionTree(rnode.child[childnode])


def createXMLFile(rnode=None, node1=None):

    try:
        attr = 'attr' + str(rnode.parent.attribute)
    except:
        for ch in rnode.child.keys():
            createXMLFile(rnode.child.get(ch), node1)
        return

    attrname = str(rnode.nodevalue)
    if bool(rnode.child) == False:
        node = ET.SubElement(node1, "node", classes=str(rnode.ulabel_count), entropy=str(rnode.entropy))
        node.text = str(rnode.leaflabel)
        node.set(attr, attrname)
        return
    node = ET.SubElement(node1, "node", classes=str(rnode.ulabel_count), entropy=str(rnode.entropy))
    node.set(attr, attrname)
    for ch in rnode.child.keys():
        createXMLFile(rnode.child.get(ch), node)
    return

# Generate the root node
rnode = Nodes(None, data_instance[:], -1)

# Get the list of labels
rnode.ulabel = rnode.getULables(data_instance)
# Now the node is generated and being populated with required data to calc the entropy and gain
createDecisionTree(rnode)

# TODO generate the xml file
attrname = str(rnode.nodevalue)
root = ET.Element("tree", classes=str(rnode.ulabel_count), entropy=str(rnode.entropy))
tree = ET.ElementTree(root)
createXMLFile(rnode, root)
tree.write("id3.xml", encoding="UTF-8", method="xml")
################################################################
print("labeled")