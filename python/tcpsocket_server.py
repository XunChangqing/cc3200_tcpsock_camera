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

address = ('192.168.1.100', 31500)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
try:
    s.bind(address)
    s.listen(True)
    
    print 'listening'
    ss, addr = s.accept()
    print 'got connected from',addr
    
    while 1:
        length = recvall(ss, 2)
        #print length
        length = struct.unpack('H', length)[0]
        print length
        stringData = recvall(ss, length)
        data = np.fromstring(stringData, dtype='uint8')
        decimg = cv2.imdecode(data, 1)
        if decimg != None:
            print decimg.shape
            cv2.imshow("server", decimg)
            cv2.waitKey(100)
        else:
            print 'decimg None'
#ra = ss.recv(512)
#print ra
except KeyboardInterrupt:
    try:
        ss.close()
        print 'close ss'
    except: print 'failed to close ss'
    try: 
        s.close()
        print 'close s'
    except: print 'faield to close s'

