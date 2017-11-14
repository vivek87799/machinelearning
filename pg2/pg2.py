import math
import csv

data_instance = list()
calc = []
ulabel = set()
ulabel_dict = dict()
dict_labelcount = dict()
features = dict()
features_label = dict()
ent = float(0)
#Calculating overall entropy
def calcEntropy (ulabel,data_instance,total_instance):
    c=len(ulabel)
    global ent
    dict_labelcount = dict.fromkeys(ulabel,0)
    for di in data_instance:
       dict_labelcount[di[6]] = dict_labelcount.get(di[6], 0) + 1

    for ul in ulabel:
       pi = float(dict_labelcount.get(ul)) / total_instance
       ent = ent - (pi * math.log(pi,c))
    
    print(ent)
 
    return;

#Calculating feature overall entropy
def calcFeatureEntropy (ulabel,data_instance,total_instance):
    global features
    c = len(ulabel)
    feature_ent = 0.0
    for di in data_instance:
       features[di[0]] = features.get(di[0], 0) + 1
       print(features)

    for f in features.keys():
       features_label.clear()
       for di in data_instance:
          if(f == di[0]):
            features_label[di[6]] = features_label.get(di[6], 0) + 1
       print(features_label)
       pi = 0.0
       feature_ent = 0.0
       gain = 0.0
       for fl in features_label:
          print(features[f])
          print(fl)
          pi = (float(features_label[fl])/features[f])
          feature_ent=feature_ent - (pi * math.log(pi,c))
          svds = float(features[f])/total_instance
          print(gain)
          gain = (svds * feature_ent)
   
          print(gain)
       gain = gain + gain
       print(gain)
       calc.append(features_label)
    print(ent)
    gain = ent - gain
    print(gain)
    print(features.keys())
    for ca in calc:
       print(ca)
    return;


# opening the data file
#a more pythonic way with efficient memory usage. Proper usage of with and file iterators. 
with open("cardaten/car.data",'rb') as dataset_file:
    data_set= csv.reader(dataset_file,delimiter=',') 
    for ds in data_set:
       data_instance.append(ds)
       print(ds[len(ds)-1])
       ulabel.add(ds[len(ds)-1])
total_instance = len(data_instance)

calcEntropy(ulabel,data_instance,total_instance)
calcFeatureEntropy(ulabel,data_instance,total_instance)

print(dict_labelcount)




