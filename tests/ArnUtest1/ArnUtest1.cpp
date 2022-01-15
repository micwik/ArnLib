#include "TestMQFlags.hpp"
#include <ArnInc/ArnM.hpp>
#include <ArnInc/ArnBasicItem.hpp>
#include <ArnInc/ArnItem.hpp>
#include <ArnItemNet.hpp>
#include <ArnInc/ArnMonitor.hpp>
#include <ArnInc/MQFlags.hpp>
#include <ArnInc/Math.hpp>
#include <ArnInc/XStringMap.hpp>
#include <ArnInc/ArnEvent.hpp>
#include <ArnInc/ArnLib.hpp>
#include <ArnInc/ArnQml.hpp>
#include <QString>
#include <QtTest>
#include <QDebug>


class ArnUtest1Sub : public QObject
{
    Q_OBJECT
public:
    ArnUtest1Sub( QObject* parent) : QObject(parent) {}

    QString  _path;

public slots:
    void  ArnErrorLog( const QString& txt);
    void  itemUpdated( const QByteArray& value);
    void  monChildFound( const QString& path);
};


class ArnUtest1 : public QObject
{
    Q_OBJECT
public:
    ArnUtest1();

private slots:
    void  initTestCase();
    void  cleanupTestCase();
    void  measureMisc1();
    void  measureMisc2();
    void  testMQFlagsText();
    void  testXStringMap();
    void  testArnEvent();
    void  testArnBasicItem1();
    void  testArnBasicItem2();
    void  testArnBasicItemDestroy();
    void  testArnItem1();
    void  testArnItem2();
    void  testArnItemDestroy();
    void  testArnItemNet1();
    void  testArnMonitorLocal();
    void  testArnQml1();

private:
    ArnUtest1Sub*  _tsub;
};


ArnUtest1::ArnUtest1()
{
    _tsub = new ArnUtest1Sub( this);
}


void ArnUtest1::initTestCase()
{
    // Arn::debugSizes = false;
    ArnM::setConsoleError( false);
    connect( &ArnM::instance(), SIGNAL(errorLogSig(QString,uint,void*)),
             _tsub, SLOT(ArnErrorLog(QString)));
    ArnM::setDefaultIgnoreSameValue( true);
}


void ArnUtest1::cleanupTestCase()
{
}


void ArnUtest1::measureMisc1()
{
    QByteArray t1;
    t1 = "qwertyuiopqwertyuiopqwertyuiop";
    QString  t2;
    QBENCHMARK {
        t2.resize(0);
        t2 += QString::fromUtf8( t1.constData());
    }
}


void ArnUtest1::measureMisc2()
{
    QString  t1;
    t1 = "qwertyuiopqwertyuiopqwertyuiop";
    QString  t2;
    QBENCHMARK {
        t2.resize(0);
        t2 += t1;
    }
}


void ArnUtest1::testMQFlagsText()
{
    AllowClassT  allow;
    // qDebug() << "AllowVal BitSet(Enum,humanized):" << allow.txt().getBitSet( allow.NsEnum);
    QVERIFY( allow.txt().getBitSet( allow.NsEnum) == "B0=Read B1=Write B2=Create B3=Delete B4=Mode_chg 0=None 3=Read_write 0xff=All");
    // qDebug() << "AllowVal BitSet(Enum):" << allow.txt().getBitSet( allow.NsEnum, true);
    QVERIFY( allow.txt().getBitSet( allow.NsEnum, true) == "B0=Read B1=Write B2=Create B3=Delete B4=ModeChg 0=None 3=ReadWrite 0xff=All");
    // qDebug() << "AllowVal1: Create=" << allow.Create;
    QVERIFY( allow.Create == 4);
    // qDebug() << "AllowVal2: CreateTxt=" << allow.txt().getTxt( allow.Create);
    QVERIFY( allow.txt().getTxtString( allow.Create) == "Create");
    allow.set( allow.Delete).set( allow.Read);
    // qDebug() << "AllowVal3: toString=" << allow.toString();
    QVERIFY( allow.toString() == "Read | Delete");
    allow.txt().setTxt("Test - Create", allow.Create, allow.NsHuman);
    allow.txt().setMissingTxt( allow.NsHuman);

    AllowClassT  allow2;
    // qDebug() << "AllowVal BitSet(Human):" << allow2.txt().getBitSet( allow2.NsHuman);
    QVERIFY( allow2.txt().getBitSet( allow2.NsHuman) == "B0=Allow_Read B1=Write B2=Test_-_Create B3=Allow_Delete B4=Mode_chg 0=None 3=Read_write 0xff=All");
    // qDebug() << "AllowVal4: getTxt=" << allow2.txt().getTxt( allow2.Create, allow2.NsHuman);
    QVERIFY( allow2.txt().getTxtString( allow2.Create, allow2.NsHuman) == "Test - Create");
    allow2 = AllowClassT::fromString("Write | Read");
    // qDebug() << "AllowVal5: toString=" << allow2.toString();
    QVERIFY( allow2.toString() == "Read | Write");
    // qDebug() << "AllowVal allow2: name=" << allow2.name();
    QVERIFY( QString( allow2.name()) == "AllowClassT");

    DataTypeT  data1;
    data1.txt().setMissingTxt( data1.NsHuman);
    // qDebug() << "DataVal EnumSet(Enum,humanized):" << data1.txt().getEnumSet( data1.NsEnum);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum) == "-2=Real 0=Null 1=Int 2=Double 3=Byte_array 4=String 5=Variant");
    // qDebug() << "DataVal EnumSet(Enum):" << data1.txt().getEnumSet( data1.NsEnum);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum, true) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
    // qDebug() << "DataVal EnumSet(Human):" << data1.txt().getEnumSet( data1.NsHuman);
    QVERIFY( data1.txt().getEnumSet( data1.NsHuman) == "-2=Real 0=Null 1=Int 2=Double 3=Bytes_type 4=String 5=Variable_type");
    data1 = DataTypeT::Real;
    // qDebug() << "DataVal1: toString=" << data1.toString() << "  val=" << data1.toInt();
    QVERIFY( data1.toString() == "Real");
    QVERIFY( data1.toInt() == -2);
    data1 = DataTypeT::fromString("Double");
    // qDebug() << "DataVal2: toString=" << data1.toString() << "  val=" << data1.toInt();
    QVERIFY( data1.toString() == "Double");
    QVERIFY( data1.toInt() == 2);
    data1.txt().setTxt("Int - Not set", data1.Int, data1.NsEnum);  // Not allowed change, do nothing
    // qDebug() << "DataVal EnumSet(Enum,humanized):" << data1.txt().getEnumSet( data1.NsEnum);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum) == "-2=Real 0=Null 1=Int 2=Double 3=Byte_array 4=String 5=Variant");
    // qDebug() << "DataVal EnumSet(Enum):" << data1.txt().getEnumSet( data1.NsEnum);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum, true) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
    data1.txt().setTxt("Int - test", data1.Int, data1.NsHuman);
    // qDebug() << "DataVal EnumSet(Human):" << data1.txt().getEnumSet( data1.NsHuman);
    QVERIFY( data1.txt().getEnumSet( data1.NsHuman) == "-2=Real 0=Null 1=Int_-_test 2=Double 3=Bytes_type 4=String 5=Variable_type");
    QString enumSet1 = data1.txt().getEnumSet( data1.NsEnum, true);
    Arn::EnumTxt etxt1;
    etxt1.loadEnumSet( enumSet1);
    // qDebug() << "DataVal EnumSet(Enum) loaded:" << etxt1.getEnumSet( 0, true);
    QVERIFY( etxt1.getEnumSet( 0, true) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
    Arn::EnumTxt etxt2;
    etxt2.loadEnumSet( "1=One 0x2=Two 10=Ten 0xb=Eleven");
    // qDebug() << "EnumSet(Enum) loaded:" << etxt2.getEnumSet( 0, true);
    QVERIFY( etxt2.getEnumSet( 0, true) == "1=One 2=Two 10=Ten 11=Eleven");
    DataTypeT  data2;
    // qDebug() << "DataVal data2 EnumSet(Human):" << data2.txt().getEnumSet( data2.NsHuman);
    QVERIFY( data2.txt().getEnumSet( data2.NsHuman) == "-2=Real 0=Null 1=Int_-_test 2=Double 3=Bytes_type 4=String 5=Variable_type");
    // qDebug() << "DataVal3 data2: toString=" << data2.toString();
    QVERIFY( data2.toString() == "Null");
    // qDebug() << "DataVal data2: name=" << data2.name();
    QVERIFY( QString( data2.name()) == "DataTypeT");

    SubEClassT  subE1;
    subE1.set( subE1.Flag2);
    subE1.setSubEnum( DataTypeT::Variant);
    subE1.setSubEnum( AbsPosT::Cepa);
    // qDebug() << "SubEClassT subE1 BitSet(Human):" << subE1.txt().getBitSet( subE1.NsEnum, true);
    QVERIFY( subE1.txt().getBitSet( subE1.NsEnum, true) == "B0=Flag1 B5=Flag2 B8=Flag3 0=None 0x121=FlagAll SE0xe:B1=DType E6-=Real E0=Null E1=Int E2=Double E3=ByteArray E4=String E5=Variant SE0x50=APos E0=None E0x10=Apa E0x40=Bepa E0x50=Cepa");
    // qDebug() << "subE1: toString=" << subE1.toString();
    QVERIFY( subE1.toString() == "Flag2 | Variant | Cepa");
    DataTypeT data3 = subE1.getSubEnum_DataTypeT();
    // qDebug() << "data3 (from subE1): toString=" << data3.toString();
    QVERIFY( data3.toString() == "Variant");
    int enumVal = 0;
    uint bitMask = 0;
    subE1.txt().getSubEnumVal( "Variant", enumVal, bitMask);
    // qDebug() << "subE1.getSubEnumVal Variant: enumVal bitMask subEnumName:" << enumVal << bitMask << subE1.txt().getTxt( bitMask);
    QVERIFY( enumVal == 10);
    QVERIFY( bitMask == 14);
    QVERIFY( subE1.txt().getTxtString( bitMask) == "DType");
    SubEClassT  subE2;
    subE2 = SubEClassT::fromString( "Bepa | Flag3 | String");
    // qDebug() << "subE2: toString=" << subE2.toString();
    QVERIFY( subE2.toString() == "Flag3 | String | Bepa");
    QString subEnumSet1 = subE1.txt().getBitSet( subE1.NsEnum, true);
    Arn::EnumTxt subEtxt1( true);
    subEtxt1.loadBitSet( subEnumSet1);
    // qDebug() << "DataVal BitSet(Flags) loaded:" << subEtxt1.getBitSet( 0, true);
    QVERIFY( subEtxt1.getBitSet( 0, true) == "B0=Flag1 B5=Flag2 B8=Flag3 0=None 0x121=FlagAll SE0xe:B1=DType E6-=Real E0=Null E1=Int E2=Double E3=ByteArray E4=String E5=Variant SE0x50=APos E0=None E0x10=Apa E0x40=Bepa E0x50=Cepa");
    // qDebug() << "DataVal BitSet(Flags) loaded numSubEnum:" << subEtxt1.subEnumCount();
    QVERIFY( subEtxt1.subEnumCount() == 2);
    const Arn::EnumTxt* etxt1a = subEtxt1.subEnumAt( 0);
    // qDebug() << "DataVal EnumSet(Enum):" << etxt1a->getEnumSet( 0, true);
    QVERIFY( etxt1a->getEnumSet( 0, true) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
    Arn::XStringMap xsm1;
    SubEClassT::txt().addFlagsTo( xsm1, Arn::EnumTxt::IncludeMode::OnlySubEnumBits, 0, true);
    // qDebug() << "SubEClassT AddFlagsTo(SubEnumBits):" << xsm1.toXString();
    QVERIFY( xsm1.toXString() == "B1=DTypeB0 B2=DTypeB1 B3=DTypeB2 B4=APosB0 B6=APosB1 0xe=DType 0x50=APos");

    // qDebug() << "log2: 0 15 1<<34 -1:" << Arn::log2(0) << Arn::log2(char(15)) << Arn::log2(long(1l << 34)) << Arn::log2(qulonglong(-1ll));

    UsageT  usage;
    // qDebug() << "Usage connectStat: string=" << usage._connectStat.toString();
    QVERIFY( usage._connectStat.toString() == "Init");
    // qDebug() << "Usage connectStat: EnumSet=" << usage._connectStat.txt().getEnumSet( UsageT::ConnectStatT::NsHuman);
    QVERIFY( usage._connectStat.txt().getEnumSet( UsageT::ConnectStatT::NsHuman)
             == "0=Initialized 1=Connecting 2=Negotiating 3=Connected 4=Stopped 5=Connect_error 6=Disconnected 7=Tried_all");
}


void  ArnUtest1::testXStringMap()
{
    using Arn::XStringMap;

    XStringMap  xsm1;
    xsm1.add("", "putValue");
    xsm1.add("Item", "/term/32/tempIs=12");
    xsm1.add("Item", "/term/32/name=Lilla rummet");
    xsm1.add("Item", "/term/32/name=Test_ \\ ^ ");
    xsm1.add("Item", "/term/32/tempSet=125");
    xsm1.add(0, 123, QByteArray("Null key-prefix"));
    QVERIFY( xsm1.key( xsm1.size(), "--UNDEF--") == "--UNDEF--");
    QVERIFY( xsm1.value( xsm1.size(), "--UNDEF--") == "--UNDEF--");
    QByteArray  xs1 = xsm1.toXString();
    // qDebug() << "XS: " << xs;
    QVERIFY( xs1 == "putValue Item=/term/32/tempIs=12 Item=/term/32/name=Lilla_rummet "
                   "Item=/term/32/name=Test\\__\\\\_\\^_ Item=/term/32/tempSet=125 123=Null_key-prefix");
    XStringMap xsm2;
    xsm2.fromXString( xs1);
    QVERIFY( xsm2.toXString() == xs1);    
    xsm1.clear();
    xsm1.addNum("a", 123);
    QVERIFY( xsm1.toXString() == "a=123");

    XStringMap xsm3;
    xsm3.add("","");
    // qDebug() << "XStringMap empty key & val: xstring=" << xsm1.toXString();
    QVERIFY( xsm3.toXString() == "=");
    xsm3.add(""," \n=_");
    // qDebug() << "XStringMap empty key & val with '=': xstring=" << xsm1.toXString();
    QVERIFY( xsm3.toXString() == "= =_\\n=\\_");
    xsm3.add(""," \nabc_");
    // qDebug() << "XStringMap empty key & val without '=': xstring=" << xsm1.toXString();
    QVERIFY( xsm3.toXString() == "= =_\\n=\\_ _\\nabc\\_");
    xsm3.add("def"," \ncba_");
    // qDebug() << "XStringMap normal key & val: xstring=" << xsm1.toXString();
    QVERIFY( xsm3.toXString() == "= =_\\n=\\_ _\\nabc\\_ def=_\\ncba\\_");
    XStringMap xsm4( xsm3.toXString());
    QVERIFY( xsm4.toXString() == "= =_\\n=\\_ _\\nabc\\_ def=_\\ncba\\_");
    QVERIFY( xsm4.key(0) == "");
    QVERIFY( xsm4.key(1) == "");
    QVERIFY( xsm4.key(2) == "");
    QVERIFY( xsm4.key(3) == "def");
    QVERIFY( xsm4.value(0) == "");
    QVERIFY( xsm4.value(1) == " \n=_");
    QVERIFY( xsm4.value(2) == " \nabc_");
    QVERIFY( xsm4.value("def") == " \ncba_");
    XStringMap xsm5( xsm3);
    QVERIFY( xsm5.toXString() == "= =_\\n=\\_ _\\nabc\\_ def=_\\ncba\\_");

    XStringMap xsm6;
    xsm6.setOptions( XStringMap::Options::NullTilde);
    QByteArray b6Apa( "a~b\0c\0\0", 7);
    xsm6.add( "apa", b6Apa);
    QByteArray b6Xstr( xsm6.toXString());
    // qDebug() << "XStringMap NullTilde: xstring=" << b6Xstr;
    QVERIFY( b6Xstr == "apa=a~b^~c~~");
    XStringMap xsm7( b6Xstr);
    QByteArray b7Apa = xsm7.value( "apa");
    QVERIFY( b7Apa == b6Apa);

    xsm6.setOptions( XStringMap::Options::RepeatLen);
    xsm6.clear();
    xsm6.add( "apa", "abbcccdddd\n\n\n");
    b6Xstr = xsm6.toXString();
    // qDebug() << "XStringMap RepeatLen: xstring=" << b6Xstr;
    QVERIFY( b6Xstr == "apa=abbc\\2d\\3\\n\\2");
}


void ArnUtest1::testArnEvent()
{
    ArnEvValueChange  ev(123, 0, ArnLinkHandle::null());
    // qDebug() << "Ev-name: " << ev.toString();
    QVERIFY( ev.toString() == "ValueChange(2022)");
}


void ArnUtest1::testArnBasicItem1()
{
    //qDebug() << "---------testArnBasicItem1 Start";
    ArnBasicItem  arnT1a;
    ArnBasicItem  arnT1b;
    arnT1a.open("//Test/Tb1/value");
    arnT1b.open("//Test/Tb1/value");
    arnT1a.setIgnoreSameValue( false);
    arnT1b.setIgnoreSameValue( false);

    arnT1a.setValue( 123);
    QCOMPARE( arnT1b.toInt(), 123);

    QBENCHMARK {
        arnT1a.setValue( 124);
    }
    //qDebug() << "---------testArnBasicItem1 End";
}


void ArnUtest1::testArnBasicItem2()
{
    ArnBasicItem  arnT1a;
    ArnBasicItem  arnT1b;
    ArnBasicItem  arnT2a;
    arnT1a.open("//Test/Tbb1/value");
    QCOMPARE( arnT1a.refCount(), 1);
    arnT1b.open("//Test/Tbb1/value");
    QCOMPARE( arnT1a.refCount(), 2);
    arnT2a.open("//Test/Tbb2/value");

    QCOMPARE( arnT1a.isIgnoreSameValue(), true);
    arnT1a.setIgnoreSameValue( false);
    QCOMPARE( arnT1a.isIgnoreSameValue(), false);
    QCOMPARE( arnT1b.isIgnoreSameValue(), true);
    arnT1b.setIgnoreSameValue( false);
    QCOMPARE( arnT1b.isIgnoreSameValue(), false);

    QCOMPARE( arnT2a.isIgnoreSameValue(), true);
    arnT2a.setPipeMode();
    QCOMPARE( arnT2a.isIgnoreSameValue(), false);
    QCOMPARE( arnT2a.isBiDirMode(), true);
    QCOMPARE( arnT2a.isPipeMode(), true);

    bool  isOk;
    arnT1a = "1234";
    isOk = false;
    QCOMPARE( arnT1a.toInt( &isOk), 1234);
    QCOMPARE( isOk, true);
    arnT1a.setValue("T1234");
    isOk = true;
    QCOMPARE( arnT1a.toInt( &isOk), 0);
    QCOMPARE( isOk, false);
    arnT1a.setValue("1234T");
    isOk = true;
    QCOMPARE( arnT1a.toInt( &isOk), 0);
    QCOMPARE( isOk, false);

    arnT1a = 0x55;
    QCOMPARE( arnT1b.toInt(), 0x55);
    arnT1b.setBits( 0xf0, 0xaa);
    QCOMPARE( arnT1a.toInt(), 0xa5);
    // arnT1b.addValue( 10);
    arnT1b += 10;
    QCOMPARE( arnT1a.toInt(), 175);
    arnT1a = 0.1;
    //arnT1b.addValue( 0.2);
    arnT1b += 0.2;
    QCOMPARE( arnT1a.toReal(), 0.3);
}


void ArnUtest1::testArnBasicItemDestroy()
{
    QVERIFY( ArnM::exist("//Test/Tb3/") == false);
    ArnM::setValue("//Test/Tb3/value", 1);
    ArnM::setValue("//Test/Tb3/xxx", 1);
    ArnBasicItem  arnT1a;
    arnT1a.open("//Test/Tb3/yyy");
    QVERIFY( ArnM::exist("//Test/Tb3/") == true);
    QVERIFY( ArnM::exist("//Test/Tb3/value") == true);
    ArnM::destroyLink("//Test/Tb3/");
    QVERIFY( ArnM::exist("//Test/Tb3/") == false);
    QVERIFY( ArnM::exist("//Test/Tb3/value") == false);
}


void  ArnUtest1::testArnItem1()
{
    ArnItem  arnT1a("//Test/Tf1/value");
    ArnItem  arnT1b("//Test/Tf1/value");
    arnT1a.setIgnoreSameValue( false);
    arnT1b.setIgnoreSameValue( false);
    // connect( &arnT1b, SIGNAL(changed(QByteArray)), _tsub, SLOT(itemUpdated(QByteArray)));

    arnT1a = 123;
    QCOMPARE( arnT1b.toInt(), 123);

    QBENCHMARK {
        arnT1a = 124;
    }
}


void  ArnUtest1::testArnItem2()
{
    ArnItem  arnT1a("//Test/Tf2/value");
    ArnItem  arnT1b("//Test/Tf2/value");
    arnT1a.setIgnoreSameValue( true);
    arnT1b.setIgnoreSameValue( true);
    //// Check Null -> int=0
    QVERIFY( arnT1b.toByteArray() == "");
    arnT1a = 0;
    // qDebug() << "testArnItem2: toByteArray()=" << arnT1b.toByteArray();
    QVERIFY( arnT1b.toByteArray() == "0");
    //// Check "" -> int=0
    arnT1a = "";
    QVERIFY( arnT1b.toByteArray() == "");
    arnT1a = 0;
    QVERIFY( arnT1b.toByteArray() == "0");

    ArnItem  arnT2aPv("//Test/Tf3/value!");
    ArnItem  arnT2a("//Test/Tf3/value");
    arnT2aPv = 0;
    arnT2a   = 0;
    QVERIFY( arnT2a.toInt() == 0);
    QVERIFY( arnT2aPv.toInt() == 0);
    arnT2aPv = 123;
    QVERIFY( arnT2a.toInt() == 123);
    QVERIFY( arnT2aPv.toInt() == 0);
    arnT2a = 321;
    QVERIFY( arnT2a.toInt() == 123);
    QVERIFY( arnT2aPv.toInt() == 321);
}


void ArnUtest1::testArnItemDestroy()
{
    QVERIFY( ArnM::exist("//Test/Td1/") == false);
    ArnM::setValue("//Test/Td1/value", 1);
    ArnM::setValue("//Test/Td1/xxx", 1);
    ArnItem  arnT1a("//Test/Td1/yyy");
    QVERIFY( ArnM::exist("//Test/Td1/") == true);
    QVERIFY( ArnM::exist("//Test/Td1/value") == true);
    ArnM::destroyLink("//Test/Td1/");
    QVERIFY( ArnM::exist("//Test/Td1/") == false);
    QVERIFY( ArnM::exist("//Test/Td1/value") == false);
}


void  ArnUtest1::testArnItemNet1()
{
    ArnItemNet  arnT2aPv(0);
    ArnItemNet  arnT2a(0);
    arnT2aPv.open("//Test/Tn1/value!");
    arnT2a.open("//Test/Tn1/value");
    arnT2aPv.arnImport("0");
    arnT2a.arnImport("0");
    QVERIFY( arnT2a.toInt() == 0);
    QVERIFY( arnT2aPv.toInt() == 0);
    arnT2aPv.arnImport("123");
    QVERIFY( arnT2a.toInt() == 0);
    QVERIFY( arnT2aPv.toInt() == 123);
    arnT2a.arnImport("321");
    QVERIFY( arnT2a.toInt() == 321);
    QVERIFY( arnT2aPv.toInt() == 123);
}


void ArnUtest1::testArnMonitorLocal()
{
    ArnMonitor  arnMon;
    ArnM::setValue("//Test/Mon/T1/value", 1);
    arnMon.start("//Test/Mon/", 0);
    connect( &arnMon, SIGNAL(arnChildFound(QString)), _tsub, SLOT(monChildFound(QString)));
    QSignalSpy spy(&arnMon, SIGNAL(arnChildFound(QString)));
    while (spy.count() == 0)
        QTest::qWait(200);
    // qDebug() << "Monitor childs 1: path=" << _tsub->_path;
    QVERIFY( _tsub->_path == "//Test/Mon/T1/");
    ArnM::setValue("//Test/Mon/T2/value", 1);
    // qDebug() << "Monitor childs 2: path=" << _tsub->_path;
    QVERIFY( _tsub->_path == "//Test/Mon/T2/");
}


void ArnUtest1::testArnQml1()
{
    Arn::QmlMSys  qmlMSysys;
    QString  xstr = "B0=Flag1 B5=Flag2 B8=Flag3 0=None 0x121=FlagAll SE0xe:B1=DType E6-=Real E0=Null E1=Int E2=Double E3=ByteArray E4=String E5=Variant SE0x50=APos E0=PNone E0x10=Apa E0x40=Bepa E0x50=Cepa";
    QVariantMap map = qmlMSysys.xstringToEnum( xstr);
    // qDebug() << "xstringToEnum QVarainMap: map=" << map;
    QString str;
    QVariantMap::const_iterator  i = map.begin();
    while (i != map.end()) {
        str += "(" + i.key() + "=" + i.value().toString() + ")";
        ++i;
    }
    // qDebug() << "xstringToEnum serialized: str=" << str;
    QVERIFY( str == "(APos=80)(Apa=16)(Bepa=64)(ByteArray=6)(Cepa=80)(DType=14)(Double=4)(Flag1=1)(Flag2=32)(Flag3=256)(FlagAll=289)(Int=2)(None=0)(Null=0)(PNone=0)(Real=12)(String=8)(Variant=10)");
}


void  ArnUtest1Sub::ArnErrorLog(const QString& txt)
{
    qDebug() << "ArnErrorLog: " << txt;
}


void  ArnUtest1Sub::itemUpdated( const QByteArray& value)
{
    Q_UNUSED(value)
    //qDebug() << "Item updated: val" << value;
}


void  ArnUtest1Sub::monChildFound( const QString& path)
{
    //qDebug() << "Monitor updated: path=" << path;
    _path = path;
}


QTEST_MAIN(ArnUtest1)

#include "ArnUtest1.moc"
