# server
import socket
import cv2
import numpy as np
import struct

def recvall(sock, count):
    buf = b''
    while count:
        newbuf = sock.recv(count)
        #print newbuf
        if not newbuf: return None
        buf += newbuf
        count -= len(newbuf)
        #print count
    return buf

address = ('192.168.1.1', 31500)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # s = socket.socket()
s.connect(address)
print "connect success!"
#s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
try:
    while 1:
        length = recvall(s, 2)
        #print length
        length = struct.unpack('H', length)[0]
        stringData = recvall(s, length)
        print length, len(stringData)
        data = np.fromstring(stringData, dtype='uint8')
        decimg = cv2.imdecode(data, 1)
        if decimg != None:
            print decimg.shape
            gray = cv2.cvtColor(decimg, cv2.COLOR_BGR2GRAY)
            ret1, thd_frame = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY)
            thd_frame_tmp = np.copy(thd_frame)
            contours, hierarchy = cv2.findContours(thd_frame_tmp, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
            for cont in contours:
                bbox = cv2.boundingRect(cont)
                cv2.rectangle(decimg, (bbox[0],bbox[1]), (bbox[0]+bbox[2],bbox[1]+bbox[3]), (0,0,255), 4)
            cv2.imshow("server", decimg)
            cv2.imshow("thd", thd_frame);
            cv2.waitKey(100)
        else:
            print 'decimg None'
#ra = ss.recv(512)
#print ra
except KeyboardInterrupt:
    try:
        s.close()
        print 'close s'
    except: print 'faield to close s'

