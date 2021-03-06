#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>
#include <QLocale>
#include <algorithm>
#include <functional>
#include <QThread>
using namespace std;

struct Nuclide
{
    QJsonObject nuclideInfo;
    QStringList *lines;
    QStringList *decays;
    QStringList *childs;
    QStringList *parents;
};

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
static QStringList* makeLinesList(QJsonArray &allLines, int nuclideNum ,QJsonArray decays){
    QStringList* lines = new QStringList();
    auto compare = [&nuclideNum](const QJsonValue &val)
    {
        return val.toObject()["Nuclides_Num"].toInt() == nuclideNum;
    };
    for(auto founder = find_if(allLines.begin(),allLines.end(),compare); founder!=allLines.end();founder  = find_if(allLines.begin(),allLines.end(),compare))
    {
        if (founder == allLines.end())
            break;
        QJsonObject currentLine = founder->toObject();
        if (currentLine["Energy"].toDouble()<=0)
        {
            allLines.erase(founder);
            continue;
        }
        QString currentLineString = QString("%1 %2 %3 %4").arg(QString::number(currentLine["Energy"].toDouble() *1000),QString::number(currentLine["DEnergy"].toDouble() *1000),
                QString::number(currentLine["I"].toDouble() /100),QString::number(currentLine["DI"].toDouble() / 100));
        lines->append(currentLineString);
        allLines.erase(founder);

    }
    qDebug() <<1;
    return lines;
}
static QStringList* makeDecaysList(QJsonArray &allDecays, int nuclideNum){
    QStringList* decays = new QStringList();
    auto compare = [&nuclideNum](const QJsonValue &val)
    {
        return val.toObject()["Nuclides_Num"].toInt() == nuclideNum;
    };
    for(auto founder = find_if(allDecays.begin(),allDecays.end(),compare); founder!=allDecays.end();founder  = find_if(allDecays.begin(),allDecays.end(),compare))
    {
        if (founder == allDecays.end())
            break;
        QJsonObject currentDecay = founder->toObject();
        if (currentDecay["Energy"].toDouble()<=0)
        {
            allDecays.erase(founder);
            continue;
        }
        QString currentDecayString = QString("%1|%2").arg(currentDecay["Decay_Mode"].toString(), QString::number(currentDecay["Decay_Branch"].toDouble()));
        decays->append(currentDecayString);
        //allDecays.erase(founder);
    }
    return decays;
}
static QStringList* makeChildsList(QJsonArray &allDecays, int nuclideNum){
    QStringList* childs = new QStringList();
    auto compare = [&nuclideNum](const QJsonValue &val)
    {
        return val.toObject()["Nuclides_Num"].toInt() == nuclideNum;
    };
    for(auto founder = find_if(allDecays.begin(),allDecays.end(),compare); founder!=allDecays.end();founder  = find_if(allDecays.begin(),allDecays.end(),compare))
    {
        if (founder == allDecays.end())
            break;
        QJsonObject currentDecay = founder->toObject();
        if (currentDecay["Energy"].toDouble()<=0)
        {
            allDecays.erase(founder);
            continue;
        }
        QString currentDecayString = QString("%1|%2|%3").arg(currentDecay["Decay_Mode"].toString() ,
                QString::number(currentDecay["Decay_Branch"].toDouble()),QString::number(currentDecay["Child_Num"].toDouble()));
        childs->append(currentDecayString);
        //allDecays.erase(founder);
    }
    return childs;
}
static QStringList* makeParentsList(QJsonArray &allDecays, int nuclideNum){
    QStringList* parents= new QStringList();
    auto compare = [&nuclideNum](const QJsonValue &val)
    {
        return val.toObject()["Child_Num"].toInt() == nuclideNum;
    };
    for(auto founder = find_if(allDecays.begin(),allDecays.end(),compare); founder!=allDecays.end();founder  = find_if(allDecays.begin(),allDecays.end(),compare))
    {
        if (founder == allDecays.end())
            break;
        QJsonObject currentDecay = founder->toObject();
        if (currentDecay["Energy"].toDouble()<=0)
        {
            allDecays.erase(founder);
            continue;
        }
        QString currentDecayString = QString("%1").arg(QString::number(currentDecay["Nuclides_Num"].toInt()));
        parents->append(currentDecayString);
       // allDecays.erase(founder);
    }
    return parents;
}
inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}
int main(int argc, char *argv[])
{
    QVector<QJsonObject> nuclidesVec;
QVector<Nuclide> nuclidesVecOut;

    QCoreApplication a(argc, argv);
    QJsonDocument docNuclides;
    QJsonDocument docDecays;
    QJsonDocument docElements;
    QJsonDocument docLines;
    QMap<int,QJsonObject> efficientLines;
    QFile nuclides("Nuclides.json");
    QFile decays("Decays.json");
    QFile output("master.mlib");
    QFile elements("Elements.json");
    QFile lines("Lines.json");
    if (!nuclides.open(QIODevice::ReadOnly) || !elements.open(QIODevice::ReadOnly) ||
            !decays.open(QIODevice::ReadOnly) || !output.open(QIODevice::ReadWrite)|| !lines.open(QIODevice::ReadOnly) )
        return 12;
    docLines = QJsonDocument::fromJson(lines.readAll());
    docElements = QJsonDocument::fromJson(elements.readAll());
    docNuclides =  QJsonDocument::fromJson(nuclides.readAll());
    docDecays = QJsonDocument::fromJson(decays.readAll());
    QJsonObject linesRoot   = docLines.object()["dataroot"].toObject();
    QJsonObject elementsRoot = docElements.object()["dataroot"].toObject();
    QJsonArray elementsArray = elementsRoot["Element"].toArray();
    QJsonObject decaysRoot =  docDecays.object()["dataroot"].toObject();
    QJsonArray decaysArray = decaysRoot["Decays"].toArray();
    QJsonObject nuclidesRoot = docNuclides.object()["dataroot"].toObject();
    QJsonArray nuclidesArray =  nuclidesRoot["Nuclides"].toArray();
    QJsonArray linesArray = linesRoot["Lines"].toArray();
    nuclides.close();
    decays.close();
    elements.close();
    lines.close();

//    sort(linesArray.begin(),linesArray.end(),[] (const QJsonValue a,const  QJsonValue b)
//    {
//        QJsonObject aO = a.toObject();
//        QJsonObject bO = b.toObject();
//        return aO["Nuclides_Num"].toInt() > bO["Nuclides_Num"].toInt();
//    });

    for (int i = 0 ; i<nuclidesArray.size(); i++)
    {


        QJsonObject parentNuclide =  nuclidesArray[i].toObject();
        //QJsonObject currentDecay = findRefInJsonArray(decaysArray,"Nuclides_Num",parentNuclide["Nuclides_Num"].toInt());

        //QJsonObject childNuclide = findRefInJsonArray(nuclidesArray,"Nuclides_Num",currentDecay["Child_Num"].toInt());
        int tsec = parentNuclide["T_sec"].toInt();
        if (tsec >3000000)
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
        qDebug()<< "????????????????????, ???????? - " << k;
    }while (!sorted ) ;
    for (int i = 0 ; i<elementsArray.size();i++)
    {
        for (int j = 0 ; j<nuclidesVec.size();j++)
        {




            if (elementsArray[i].toObject()["Element"].toString() == nuclidesVec[j]["Name"].toString())
            {

                nuclidesVecOut.push_back({nuclidesVec[j]});
            }
        }
     qDebug()<< "?????????????????????? - " << i*100/elementsArray.size() << "%";
    }
    for(int i = 0; i < nuclidesVecOut.size();i++)
    {
        QStringList* lines = new QStringList();
        QStringList* decays= new QStringList();
        QStringList* childs= new QStringList();
        QStringList* parents= new QStringList();
        QThread* linesCount = QThread::create([&lines, &linesArray,&nuclidesVecOut,i,decaysArray](){
            lines= makeLinesList(linesArray,nuclidesVecOut[i].nuclideInfo["Nuclides_Num"].toInt(),decaysArray);
                          });
        QThread* decaysCount= QThread::create([&decaysArray,&nuclidesVecOut,i,&decays](){
            decays = makeDecaysList(decaysArray,nuclidesVecOut[i].nuclideInfo["Nuclides_Num"].toInt());
                          });
        QThread*  childsCount= QThread::create([&decaysArray,&nuclidesVecOut,i,&childs](){
            childs = makeChildsList(decaysArray,nuclidesVecOut[i].nuclideInfo["Nuclides_Num"].toInt());
                          });
        QThread* parentsCount= QThread::create([&decaysArray,&nuclidesVecOut,i,&parents](){
            parents = makeParentsList(decaysArray,nuclidesVecOut[i].nuclideInfo["Nuclides_Num"].toInt());
                          });


         linesCount->start();
         decaysCount->start();
         childsCount->start();
         parentsCount->start();

         while(!(linesCount->isFinished() &&  decaysCount->isFinished() && childsCount->isFinished() && parentsCount->isFinished()))
         {

         }
//         delete linesCount;
//         delete decaysCount;
//         delete childsCount;
//         delete parentsCount;
        nuclidesVecOut[i].lines = lines;
        nuclidesVecOut[i].decays = decays;
        nuclidesVecOut[i].childs = childs;
        nuclidesVecOut[i].parents = parents;
//        delete lines;

//        delete decays;
//        delete childs;
//        delete parents;
        qDebug() << "?????????? ?????????? - " << i*100/nuclidesVecOut.size() << "%" ;
    }



    int index =0;
    for (auto item : nuclidesVecOut)
    {
        double tsec = item.nuclideInfo["T_sec"].toDouble();
        if (tsec == 1.79769313486231e+308)
        {
            tsec = 0;

        }

        QString decayModeStr;
        decayModeStr.append(" [");
       for (auto decay: *item.decays)
       {
           if (decay != item.decays->first())
                decayModeStr.append("%");
           decayModeStr.append(decay);

       }
       decayModeStr.append("]");

       QString childsModeStr;
       childsModeStr.append(" [");
      for (auto child: *item.childs)
      {
          if (child != item.childs->first())
               childsModeStr.append("%");
          childsModeStr.append(child);

      }
      childsModeStr.append("]");

      QString parentsModeStr;
      parentsModeStr.append(" [");
     for (auto parent: *item.parents)
     {
         if (parent != item.parents->first())
              parentsModeStr.append("%");
         parentsModeStr.append(parent);

     }
     parentsModeStr.append("] ");

        output.write((QString::number(item.nuclideInfo["A"].toDouble()) + " " +  QString::number(item.nuclideInfo["Z"].toDouble())  + " " +
            item.nuclideInfo["Full_Name"].toString() + " " + QString::number(item.nuclideInfo["AtomicMass"].toDouble())+" "
           + QString::number(tsec)+ " " + QString::number(item.nuclideInfo["DT_Plus"].toDouble()) + " " + QString::number(item.lines->size())
                + decayModeStr + childsModeStr + parentsModeStr + QString::number(index) + "\n").toUtf8());
        for (auto line : *item.lines)
        {
            output.write((line+"\n").toUtf8());
        }
        index++;
    }

    output.close();
    return a.exec();
}
