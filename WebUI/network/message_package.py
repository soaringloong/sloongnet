'''
@Author: WCB
@Date: 2019-11-14 13:59:50
@LastEditors  : WCB
@LastEditTime : 2019-12-24 16:20:54
@Description: Message package module
'''
from protocol import protocol_pb2 as protocol
from . import connect_session

class MessagePackage:
    pack = protocol.DataPackage()
    def __init(self):
        self.pack.Sender = "0"

    def Request(self, session, func, msg, extend= b'' ):
        self.pack.Function = func
        self.pack.Content = msg
        self.pack.Extend = extend
        (res,recv_data) = session.send(self.pack.SerializeToString())

        if not res:
            return (protocol.ResultType.Error, recv_data)
        recv_pack = protocol.DataPackage()

        recv_pack.ParseFromString(recv_data)
        return (recv_pack.Result,recv_pack.Content)