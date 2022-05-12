#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>


static QJsonObject findRefInJsonArray(QJsonArray array, QString refName, int refValue )
{
    for (auto item: array)
    {
        if (item.toObject()[refName].toInt() == refValue)
        {
            return item.toObject();
        }
    }
    return QJsonObject{};
}

int main(int argc, char *argv[])
{
    QVector<QJsonObject> nuclidesVec;
QVector<QJsonObject> nuclidesVecOut;

    QCoreApplication a(argc, argv);
    QJsonDocument docNuclides;
    QJsonDocument docDecays;
    QJsonDocument docElements;
    QFile nuclides("Nuclides.json");
    QFile decays("Decays.json");
    QFile output("master.mlib");
    QFile elements("Elements.json");
    if (!nuclides.open(QIODevice::ReadOnly) || !elements.open(QIODevice::ReadOnly) || !decays.open(QIODevice::ReadOnly) || !output.open(QIODevice::ReadWrite))
        return 12;
    docElements = QJsonDocument::fromJson(elements.readAll());
    docNuclides =  QJsonDocument::fromJson(nuclides.readAll());
    docDecays = QJsonDocument::fromJson(decays.readAll());
    QJsonObject elementsRoot = docElements.object()["dataroot"].toObject();
    QJsonArray elementsArray = elementsRoot["Element"].toArray();
    QJsonObject decaysRoot =  docDecays.object()["dataroot"].toObject();
    QJsonArray decaysArray = decaysRoot["Decays"].toArray();
    QJsonObject nuclidesRoot = docNuclides.object()["dataroot"].toObject();
    QJsonArray nuclidesArray =  nuclidesRoot["Nuclides"].toArray();
    nuclides.close();
    decays.close();



    for (int i = 0 ; i<nuclidesArray.size(); i++)
    {


        QJsonObject parentNuclide =  nuclidesArray[i].toObject();
        //QJsonObject currentDecay = findRefInJsonArray(decaysArray,"Nuclides_Num",parentNuclide["Nuclides_Num"].toInt());

        //QJsonObject childNuclide = findRefInJsonArray(nuclidesArray,"Nuclides_Num",currentDecay["Child_Num"].toInt());
        int tsec = parentNuclide["T_sec"].toInt();
        if (tsec ==1.79769313486231e+308)
        {
            tsec = 0;

        }

        if ((parentNuclide["Z"].toInt()>0||parentNuclide["A"].toInt()>0) )
            nuclidesVec.push_back(parentNuclide);


    }


    int k = 0;
    bool sorted = true;
    do{
        sorted = true;
        for (int i = 0;i<nuclidesVec.size()-1;i++)
        {
            if (!(nuclidesVec[i]["Z"].toInt()>0||nuclidesVec[i]["A"].toInt()>0) ||
                   !(nuclidesVec[i]["Z"].toInt()>0||nuclidesVec[i]["A"].toInt()>0) )
                continue;

            QStringList name = nuclidesVec[i]["Full_Name"].toString().split("-");
            QStringList nameNext = nuclidesVec[i+1]["Full_Name"].toString().split("-");
            if (name[1].contains('M') && !nameNext[1].contains('M')  )
            {
                QJsonObject tmp = nuclidesVec[i];
                nuclidesVec[i] = nuclidesVec[i+1];
                nuclidesVec[i+1] = tmp;
                sorted = false;
                continue;
            }
            else if (name[1].contains('M')  && nameNext[1].contains('M'))
            {
                QStringList number = name[1].split('M');
                 QStringList numberNext = nameNext[1].split('M');
                if (number.size()==1 )
                {
                    number.append("0");
                }
                if (numberNext.size()==1)
                {
                    numberNext.append("0");
                }
                int num = ((number[0] + number[1])).toInt();
                int numNext = ((numberNext[0] + numberNext[1])).toInt();
                if (num>numNext)
                {
                    QJsonObject tmp = nuclidesVec[i];
                    nuclidesVec[i] = nuclidesVec[i+1];
                    nuclidesVec[i+1] = tmp;
                    sorted = false;
                }

            }
            else if(!name[1].contains('M')  && !nameNext[1].contains('M'))
            {
                int num =  name[1].toInt();
                int numNext = nameNext[1].toInt();
                if (num>numNext)
                {
                    QJsonObject tmp = nuclidesVec[i];
                    nuclidesVec[i] = nuclidesVec[i+1];
                    nuclidesVec[i+1] = tmp;
                    sorted = false;
                }

            }
        }
        k++;
        qDebug()<< "Сортировка, цикл - " << k;
    }while (!sorted ) ;
    for (int i = 0 ; i<elementsArray.size();i++)
    {
        for (int j = 0 ; j<nuclidesVec.size();j++)
        {
        if (elementsArray[i].toObject()["Element"].toString() != nuclidesVec[j]["Name"].toString())
            continue;
        nuclidesVecOut.push_back(nuclidesVec[j]);
        }
     qDebug()<< "Группировка - " << i*100/elementsArray.size() << "%";
    }

    for (auto item : nuclidesVecOut)
    {
        int tsec = item["T_sec"].toInt();
        if (tsec ==1.79769313486231e+308)
        {
            tsec = 0;

        }
        output.write((QString::number(item["A"].toInt()) + " " +  QString::number(item["Z"].toInt())  + " " +
            item["Full_Name"].toString() + " " + QString::number(item["AtomicMass"].toInt())+" "
           + QString::number(tsec)+ " " + QString::number(item["DT_Plus"].toInt()) + "\n").toUtf8() );
    }

    output.close();
    return a.exec();
}
