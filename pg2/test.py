

pi = float;
total_instance = float(7);
#Calculating the entropy
def calcEntropy (ulabel,label):
    total_instance = len(label)
    print (total_instance)
    for ul in ulabel:
       print(5 / 5)
       pi = float(label.count(ul)) / total_instance
       print(ul + "==")
       print(label.count(ul))
       print(pi)
    return pi;



data_instance = list();
label = list();
try:
   # opening the data file

   #a more pythonic way with efficient memory usage. Proper usage of with and file iterators. 
   with open("cardaten/car.data") as file:
       for line in file:
          line = line.strip() #preprocess line removes the empty line
          data_instance.append(line)
          #doSomethingWithThisLine(line) #take action on line instead of storing in a list. more memory efficient at the cost of execution speed.
   
   for di in data_instance:
      feature = di.split(",")
      label.append(feature[len(feature)-1])
      print(feature[0] +" "+ feature[len(feature)-1])
      
   
   ulabel = set(label)
   for ul in ulabel:
      print(ul)
      print(label.count(ul))


   pi = calcEntropy(ulabel,label)
   print(pi)
   
   print("test")
   print(ulabel)
   print(label.count("acc"))
   print(len(label))


finally:
   file.close()
