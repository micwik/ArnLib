#ifndef TESTMQFLAGS_HPP
#define TESTMQFLAGS_HPP

#include <ArnInc/MQFlags.hpp>


class AllowClassT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None      = 0x00,
        Read      = 0x01,
        Write     = 0x02,
        Create    = 0x04,
        Delete    = 0x08,
        ModeChg   = 0x10,
        ReadWrite = 0x03,
        //! Convenience, allow all
        All       = 0xff
    };
    MQ_DECLARE_FLAGSTXT( AllowClassT)


    enum NS {NsEnum, NsHuman};

    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Read,   "Allow Read" },
        { NsHuman, Delete, "Allow Delete" }
    )
};
MQ_DECLARE_OPERATORS_FOR_FLAGS( AllowClassT)

class DataTypeT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        Null       = 0,
        Int        = 1,
        Double     = 2,
        Real       = -2,
        ByteArray  = 3,
        String     = 4,
        Variant    = 5
        // 16 and above (max 255) is reserved by ArnItemB::ExportCode
    };
    MQ_DECLARE_ENUMTXT( DataTypeT)

    enum NS {NsEnum, NsHuman};

    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, ByteArray, "Bytes type" },
        { NsHuman, Variant,   "Variable type" }
    )
};

namespace ArnT {
class ConnectStatT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        Init = 0,
        Connecting,
        Negotiating,
        Connected,
        Stopped,
        Error,
        Disconnected,
        TriedAll
    };
    MQ_DECLARE_ENUMTXT( ConnectStatT)

    enum NS {NsEnum, NsHuman};
    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Init,     "Initialized" },
        { NsHuman, Error,    "Connect error" },
        { NsHuman, TriedAll, "Tried all" },
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};
}

class UsageT {
public:
    typedef ArnT::ConnectStatT  ConnectStatT;

    ConnectStatT  _connectStat;
};

#endif // TESTMQFLAGS_HPP

