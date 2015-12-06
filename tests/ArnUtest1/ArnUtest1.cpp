#include "TestMQFlags.hpp"
#include <ArnInc/ArnM.hpp>
#include <ArnInc/MQFlags.hpp>
#include <ArnInc/ArnLib.hpp>
#include <QString>
#include <QtTest>
#include <QDebug>


class ArnUtest1Sub : public QObject
{
    Q_OBJECT
public:
    ArnUtest1Sub( QObject* parent) : QObject(parent) {}

public slots:
    void  ArnErrorLog(const QString& txt);
};


class ArnUtest1 : public QObject
{
    Q_OBJECT
public:
    ArnUtest1();

private slots:
    void  initTestCase();
    void  cleanupTestCase();
    void  testMQFlagsText();

private:
    ArnUtest1Sub*  _tsub;
};


ArnUtest1::ArnUtest1()
{
    _tsub = new ArnUtest1Sub( this);
}


void ArnUtest1::initTestCase()
{
    Arn::debugSizes = false;
    ArnM::setConsoleError( false);
    connect( &ArnM::instance(), SIGNAL(errorLogSig(QString,uint,void*)),
             _tsub, SLOT(ArnErrorLog(QString)));
    ArnM::setDefaultIgnoreSameValue( true);
}


void ArnUtest1::cleanupTestCase()
{
}


void ArnUtest1::testMQFlagsText()
{
    AllowClassT allow;
    // qDebug() << "AllowVal BitSet(Enum):" << allow.txt().getBitSet( allow.NsEnum);
    QVERIFY( allow.txt().getBitSet( allow.NsEnum) == "B0=Read B1=Write B2=Create B3=Delete B4=ModeChg");
    // qDebug() << "AllowVal1: Create=" << allow.Create;
    QVERIFY( allow.Create == 4);
    // qDebug() << "AllowVal2: CreateTxt=" << allow.txt().getTxt( allow.Create);
    QVERIFY( QString( allow.txt().getTxt( allow.Create)) == "Create");
    allow.set( allow.Delete).set( allow.Read);
    // qDebug() << "AllowVal3: toString=" << allow.toString();
    QVERIFY( allow.toString() == "Read | Delete");
    allow.txt().setTxt("Test - Create", allow.Create, allow.NsHuman);
    allow.txt().setMissingTxt( allow.NsHuman);
    AllowClassT allow2;
    // qDebug() << "AllowVal BitSet(Human):" << allow2.txt().getBitSet( allow2.NsHuman);
    QVERIFY( allow2.txt().getBitSet( allow2.NsHuman) == "B0=Allow_Read B1=Write B2=Test_-_Create B3=Allow_Delete B4=ModeChg");
    // qDebug() << "AllowVal4: getTxt=" << allow2.txt().getTxt( allow2.Create, allow2.NsHuman);
    QVERIFY( QString( allow2.txt().getTxt( allow2.Create, allow2.NsHuman)) == "Test - Create");
    allow2 = AllowClassT::fromString("Write | Read");
    // qDebug() << "AllowVal5: toString=" << allow2.toString();
    QVERIFY( allow2.toString() == "Read | Write");
    DataTypeT data1;
    data1.txt().setMissingTxt( data1.NsHuman);
    // qDebug() << "DataVal EnumSet(Enum):" << data1.txt().getEnumSet( data1.NsEnum);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
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
    // qDebug() << "DataVal EnumSet(Human):" << data1.txt().getEnumSet( data1.NsHuman);
    QVERIFY( data1.txt().getEnumSet( data1.NsEnum) == "-2=Real 0=Null 1=Int 2=Double 3=ByteArray 4=String 5=Variant");
    data1.txt().setTxt("Int - test", data1.Int, data1.NsHuman);
    // qDebug() << "DataVal EnumSet(Human):" << data1.txt().getEnumSet( data1.NsHuman);
    QVERIFY( data1.txt().getEnumSet( data1.NsHuman) == "-2=Real 0=Null 1=Int_-_test 2=Double 3=Bytes_type 4=String 5=Variable_type");
    DataTypeT data2;
    // qDebug() << "DataVal data2 EnumSet(Human):" << data2.txt().getEnumSet( data2.NsHuman);
    QVERIFY( data2.txt().getEnumSet( data2.NsHuman) == "-2=Real 0=Null 1=Int_-_test 2=Double 3=Bytes_type 4=String 5=Variable_type");
    // qDebug() << "DataVal3 data2: toString=" << data2.toString();
    QVERIFY( data2.toString() == "Null");
}


void ArnUtest1Sub::ArnErrorLog(const QString& txt)
{
    qDebug() << "ArnErrorLog: " << txt;
}


QTEST_APPLESS_MAIN(ArnUtest1)

#include "ArnUtest1.moc"
