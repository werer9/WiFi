#!/usr/local/bin/python3
# 
#   Name: Caelan Murch
#   Project: WiFiProjectServer
#   Date: 19/12/2018
#   
#   Project description:
#   Server to receive RSSI information from
#   several MCUs, print it to the terminal real time
#   and store the output in a text file.
# 
#   (c) Caelan Murch

# import required libraries
import socketserver
import datetime


# UDP handler from request handler subclass
class UDPHandler(socketserver.BaseRequestHandler): 
    
    # process request from client
    def handle(self):
        # Get data from client
        self.data = self.request[0].strip()
        # get socket used by client
        # socket = self.request[1]
        # print data from client in file and output
        print("Printing from {}: ".format(self.client_address[0]))
        print(self.data.decode('utf-8'))
        (sender, list) = self.processData(self.data.decode('utf-8'))
        self.logData("log", sender, list)
        # send reply to client
        # socket.sendto("Thanks!".encode('utf-8'), self.client_address)

    # bubble sort RSSI list
    def __bubbleSort(self, array):
        # swap array positions
        def swap(i, j):
            array[i], array[j] = array[j], array[i]
        # iterate for length of array
        count = 0
        while count < len(array):
            # check if every element needs to be swapped
            for i in range(0, len(array)-1):
                if int(array[i][0:array[i].find('R')]) > int(array[i+1][0:array[i+1].find('R')]):
                    swap(i, i+1)
            count = count + 1
        return array

    # process text
    def processData(self, text):
        # split new lines into separate arrays 
        split = text.split('\r\n')
        # find sender of message and tx power
        if split[0].find('Hello from Measure') != -1:
            sender = int(split[0][split[0].find('Hello from Measure') + 18:len(split[0])]) + \
                100*int(split[0][split[0].find('TX')+2:split[0].find('Hello from Measure')-1])
        # create list sorting the RSSI and SSID
        list = []
        for i in range(1, len(split)):
            # if word 'Measure' found is SSID
            if split[i].find('Measure') != -1:
                # find integer after Measure
                index = int(split[i][split[i].find('Measure') + 7:split[i].find('RSSI:')])
                # make it the first integer is the string follor by R
                # and then the RSSI integer
                list.append(str(index) + "R" + str(split[i][split[i].find('RSSI:') + 6:len(split[i])]))
        # order list so data is in order of integer after Measure
        list = self.__bubbleSort(list)
        return (sender, list)

    # log data
    def logData(self, filename, sender, list):
        # get time and date
        now = datetime.datetime.now()
        day = now.strftime("(%H00hrs-%d-%m-%Y)")
        time = now.strftime("[%H:%M:%S:%f] ")
        # create filename based on data
        filename = filename + " - " + day + ".txt"
        # split sender and tx power
        tx = (sender / 100)/4.0
        send = int(str(sender)[2:len(str(sender))])
        # open log file
        file = open(filename, "a")
        file.write(time + "From: Measure" + str(send) + " TX Power: {0:.1f}".format(tx) + "\n")
        # write a line with SSID and RSSI for each item in list
        for i in range(0, len(list)):
            file.write(time + "SSID: Measure" + list[i][0:list[i].find('R')] + " RSSI: " + \
                list[i][list[i].find('R')+1:len(list[i])] + "\n")
        # close file
        file.close()


# Only execute if this file is called directly
if __name__ == "__main__":
    # Set host as self and port as 8787
    HOST, PORT = '', 8787

    # Run server until interrupt
    print("Starting server...")
    with socketserver.UDPServer((HOST, PORT), UDPHandler) as server:
        print("Server started")
        server.serve_forever()
        
