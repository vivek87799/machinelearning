import math
import csv

pi = float;
total_instance = float(7);

class NodeFeature:
      values = list()
      uvalues = set()
      uvalueentropy = dict()
      uvaluecount = dict()
      def __init__(self):
        self.number = 1
      
#Calculating overall entropy
def calcEntropy (ulabel,label,total_instance):
    print (total_instance)
    ent = 0.0
    c = len(ulabel)
    for ul in ulabel:
       pi = float(label.count(ul)) / total_instance
       ent=ent - (pi * math.log(pi,c))
       print(ul + "==")
       print(label.count(ul))
       print(ent)
    return ent;

#Calculating entropy for features
def calcFeatureEntropy (ulabel,data_instance,total_instance):
    feature = []

    d2 = dict.fromkeys(ulabel,0)
    #d3 = dict.fromkeys(['first', 'second'])
    #print(d2.keys())
    for di in data_instance:
      instance = di.split(",")
      ins = len(instance)
      d2[instance[ins-1]] = d2.get(instance[ins-1], 0) + 1
      for f in range(len(instance)):
        print(instance[f])

  
      #print(d2)
    return;


data_instance = list();
label = list();
l= list();
testlabel = list();
feature1 = list();
feature2 = list();
feature3 = list();
feature4 = list();

try:
   # opening the data file
   #a more pythonic way with efficient memory usage. Proper usage of with and file iterators. 
   with open("cardaten/car.data") as file:
       for line in file:
          line = line.strip() #preprocess line removes the empty line
          data_instance.append(line)
   for di in data_instance:
      print(di)
      tl = list();
      feature = di.split(",")
      for f in feature:
        tl.append(f)

      testlabel.append(tl)
      label.append(feature[len(feature)-1])
      feature1.append(feature[0])
      feature2.append(feature[1])
      feature3.append(feature[2])
      feature4.append(feature[3])
      
   

   features_len = len(data_instance[0].split(","))-1
   #Calc the entropy
   ulabel = set(label)
   total_instance = len(label)
   entropy = calcEntropy(ulabel,label,total_instance)

   #calc the entropy of the attributes
   calcFeatureEntropy (ulabel,data_instance,total_instance)

   node = [None]* (len(data_instance[0].split(","))-1)
   
   nodes = []

   for i in range(features_len):
      nodes.append(NodeFeature)
      

   for n in nodes:
      n.uvalues= set(ulabel)
      print(n)
      print(n.uvalues)

   nb = NodeFeature()
   nb.values = feature1
   nb.uvalues = set(nb.values)
   node[1] = nb

   print(entropy)
   print("test")
   print(testlabel)
   for i in range (6):
      print(testlabel[1][i])
   print Matrix[0][0] # prints 1
   x, y = 0, 6 
   print Matrix[x][y] # prints 3; be careful with indexing! 
   print(len(nodes))
   print(label.count("acc"))
   print(len(label))



finally:
   file.close()
