# -*- coding: utf-8 -*-
"""
Created on Sun Nov 12 15:57:33 2017

@author: Ali
"""
from __future__ import division
import pandas as pd
import math
import operator
import xml.etree.ElementTree as ET

class Nodes:
    """each object of the class will represent a node in
    the decision tree. Attributes means the one holding the node and 
    value is the branch that will connect node to its subtree. Classification
    data tells us about how many rows belongs to each classifier for the node
    and its subtree. Entropy is the calculation of impurity in data"""
    def __init__(self, attribute=None, results=None, value=None,
                 subtree=None, entropy=None, classification_data=None):
        # attribute contains the name of the node
        self.attribute = attribute
        self.results = results
        # value contains the variables for the attribute
        self.value = value
        self.subtree = subtree
        self.entropy = entropy
        self.classification_data = classification_data


def calculate_entropy(dataframe):
    length_of_data = len(dataframe)
    entropy = 0
    possible_classifications = dataframe['classification'].drop_duplicates()
    #print (possible_classifications)
    each_classifier_rows = []
    for x in possible_classifications:
        x_records = len(dataframe[dataframe["classification"] == x])
        each_classifier_rows.append(x_records)
        proportion_of_x = x_records/length_of_data
        entropy = entropy - (proportion_of_x * math.log(proportion_of_x,4))
        #print(each_classifier_rows)
    return entropy,zip(possible_classifications,each_classifier_rows)

def calculate_gain(dataframe, attr_list):
    """calculate_gain calculates inforamtion gain from the dataframe for the 
    attributes in attr_list. It returns a dictionary containing the entropy value and
    the information gains for all the attributes under consideration for the node"""
    #temp_dict and temp_dict1 are temporary variables for function body
    temp_dict = {}
    temp_dict1 = {}
    entropy_value, classification_classes = calculate_entropy(dataframe)
    temp_dict["Entropy"] = entropy_value
    temp_dict["classification_data"] = classification_classes
    total_data = len(dataframe)
    if entropy_value != 0:
        for x in attr_list:
            x_values = dataframe[x].drop_duplicates()
            information_gain = entropy_value
            for val in x_values:
                #val_proportion is the fraction of examples belonging to this value 'val' of attribute 'x'
                val_proportion = len(dataframe[dataframe[x]==val])/total_data
                records_with_val = dataframe[dataframe[x]==val]
                entropy_result, class_result = calculate_entropy(records_with_val)
                information_gain = information_gain - (val_proportion * entropy_result)
            temp_dict1[x] = information_gain
            """key 'node options' of the dictionary contains a subdictionary containing information gains
            for each attribute in key-value form"""
            temp_dict["node_options"] = temp_dict1
    else:
        temp_dict["classifier"] = dataframe["classification"].drop_duplicates().iloc[0]
    return temp_dict


def getsubtree(dataframe, attr_list):
    """getsubtree is our main function which builds the whole decision tree by utilizing
    the calculations from calculate_entropy and calculate_gain and it returns a tree of nodes
    """
    dictt = calculate_gain(dataframe, attr_list)
    if 'classifier' not in dictt:
        #if classifier is not present in dictionary then it means it is not a leaf node
        """attr_with_max_gain is the attribute with the maximum information gain
        and it will occupy the node under consideration"""
        attr_with_max_gain = max(dictt["node_options"].items(), key=operator.itemgetter(1))[0]
        attr_with_max_gain_values = dataframe[attr_with_max_gain].drop_duplicates()
        attr_list.remove(attr_with_max_gain)
        resultslist = []
        for value in attr_with_max_gain_values:
            df = dataframe[dataframe[attr_with_max_gain] == value]
            resultslist.append(getsubtree(df, attr_list))
        attr_list.append(attr_with_max_gain)
        return Nodes(attribute=attr_with_max_gain, value=attr_with_max_gain_values, subtree=resultslist,
                            entropy=dictt["Entropy"], classification_data=dictt["classification_data"])
    else:
        #it is a leaf node
        return Nodes(results = dictt["classifier"],entropy=dictt["Entropy"], classification_data=dictt["classification_data"])

def calculateclassesdata(classification_data):
    """calculateclassesdata serializes the classifiers data for each node
    in required format i-e like acc:30,vgood:12,good:6"""
    output = ""
    for classifier,no_0f_rows in classification_data:
        temp = classifier+":"+str(no_0f_rows)
        output = ','.join([output,temp])
    return output[1:]

def getdictionary(tree,indent='',diction={}):
    """returns the whole tree in form of a dictionary"""
    if tree.results!=None:
        # if this is a leaf node?
        result_dict={}
        result_dict["classifier"] = tree.results
        result_dict["Entropy"] = tree.entropy
        result_dict["classification_data"] = calculateclassesdata(tree.classification_data)
        return result_dict
    else:
        diction["Entropy"] = tree.entropy
        diction["classification_data"] = calculateclassesdata(tree.classification_data)
        diction[tree.attribute]={}
        temp = zip(tree.value, tree.subtree)
        for val,sub in temp:
            diction[tree.attribute][val]={}
            tempor= getdictionary(sub,indent+'  ',diction = diction[tree.attribute][val])
            del diction[tree.attribute][val]
            diction[tree.attribute][val] = tempor
        return diction

def getxmlfromDictionary(dictionary, xml_element):
    """getxmlfromDictionary makes xml from dictionary using builtin xml.etree.ElementTree
    library"""
    xml_element.set("Entropy", str(dictionary["Entropy"]))
    xml_element.set("Classes", dictionary["classification_data"])
    del dictionary["Entropy"]
    del dictionary["classification_data"]
    if 'classifier' not in dictionary:
        for key in dictionary:
            for i in dictionary[key]:
                elem = ET.SubElement(xml_element, "node")
                elem.set(key, i)
                getxmlfromDictionary(dictionary[key][i], elem)        
    else:
        xml_element.text = dictionary['classifier']
        return

def outputXMLfile(dictionary, root):
    """outputXMLfile outputs learned tree in the form of xml file"""
    getxmlfromDictionary(dictionary, root)
    tree = ET.ElementTree(root)
    tree.write("output.xhtml")


def main(filename):

    dataframe = pd.read_csv(filename, header=None, names=["buying", "maint", "doors", "persons", "lug_boot", "safety", "classification"])
    attributes_list = [x for x in dataframe.columns if x != "classification"]
    tree = getsubtree(dataframe, attributes_list)
    tree_ditionary= getdictionary(tree)
    root = ET.Element('tree')
    outputXMLfile(tree_ditionary, root)
    

main("car.data")