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

class AbsPosT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None = 0x00,
        Apa  = 0x10,
        Bepa = 0x40,
        Cepa = 0x50
    };
    MQ_DECLARE_ENUMTXT( AbsPosT)

    enum NS {NsEnum, NsHuman};
    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};

class SubEClassT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None    = 0x000,
        Flag1   = 0x001,
        DTypeB0 = 0x002,
        DTypeB1 = 0x004,
        DTypeB2 = 0x008,
        APosB0  = 0x010,
        Flag2   = 0x020,
        APosB1  = 0x040,
        Flag3   = 0x100,
        //! SubEnum DataType
        DType   = DTypeB0 | DTypeB1 | DTypeB2,
        APos    = APosB0 | APosB1,
        //! Convenience, all flags
        FlagAll = Flag1 | Flag2 | Flag3
    };
    MQ_DECLARE_FLAGSTXT( SubEClassT)

    MQ_DECLARE_SUBETXT(
        MQ_SUBETXT_ADD_RELDEF( DataTypeT, DType, DTypeB0),
        MQ_SUBETXT_ADD_ABSDEF( AbsPosT, APos)
    )
    MQ_SUBETXT_ADD_RELOP( DataTypeT, DType, DTypeB0)
    MQ_SUBETXT_ADD_ABSOP( AbsPosT, APos)

    enum NS {NsEnum, NsHuman};
    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Flag1, "Flag nr1" },
        { NsHuman, DType, "The data type" },
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};
MQ_DECLARE_OPERATORS_FOR_FLAGS( SubEClassT)

class UsageT {
public:
    typedef ArnT::ConnectStatT  ConnectStatT;

    ConnectStatT  _connectStat;
};

#endif // TESTMQFLAGS_HPP

