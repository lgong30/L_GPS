#ifndef PACKET_SCHEDULER_HPP
#define PACKET_SCHEDULER_HPP

#include <stdexcept> // for runtime_error
#include <fstream>
#include <vector>
#include <string> // for string & getline

//#include "packet.hpp"
#include "L_GPSsim.hpp" // for Packet, Flow, GPSSim 
#include "json.hpp"

using json = nlohmann::json;
//! packet scheduler class
class L_GPS_Tester{
    L_GPSSim *L_GPSsimulator;
    //! vector for Packets
    std::vector<Packet *> mPackets;
    std::vector<double> mFlowWeights;
	std::vector<double> mFlowLastDepartVTimes;
    json Packet2JSON(int i)
    {
       assert(i >=0 && i < mPackets.size());
       json j = {
        {"flowId",mPackets[i]->mFlowId},
        {"packetId",mPackets[i]->mPacketId},
        {"arrivalTime",mPackets[i]->mArrivalTime},
        {"packetLength",mPackets[i]->mLength},
        {"virtualFinishTime",mPackets[i]->mGPS_VFTime}
       };
       return j;
    }
public:
    //! constructor
    L_GPS_Tester(std::string input){
        // input file stream
        std::ifstream infile;
        std::string lines;
        
        
        int flowNum = -1;
        std::string flowWeightConf;
        bool isEqualWeight;
        double flowWeight;
        int flowId, packetId, packetLength;
        long int arrivalTime;
        char c;
        
        // start processing input file
        try {
            
            //! open the file
            infile.open (input);
            
            //! try to read flow configuration
            while (infile >> c)
            {
                switch(c)
                {
                    case 'f':// flow description
                        if(infile >> flowNum >> flowWeightConf)
                        {
                            std::getline(infile, lines);// pass through this line
                            if(flowWeightConf=="eq") isEqualWeight = true;
                            else if(flowWeightConf=="neq") isEqualWeight = false;
                            else throw new std::runtime_error("Unknown flow flowWeight configuration.");
                        }
                        else
                        {
                            throw new std::runtime_error("Missing or wrong flow configuration.");
                        }
                        break;
                    case 'c':// comments
                        std::getline(infile, lines);// skip this line
                        break;
                    default:// unknown
                        throw new std::runtime_error("Unknown declaration.");
                }
                if (flowNum > 0) break;
            }
            
            if (!isEqualWeight)
            {// read flow weights
                while (infile >> c)
                {
                    switch(c)
                    {
                        case 'w':// weights line
                        {
                            while (mFlowWeights.size() < flowNum && infile >> flowWeight)
                                mFlowWeights.push_back(flowWeight);
                            std::getline(infile,lines);
                            if (mFlowWeights.size() < flowNum)
                                throw new std::runtime_error("Missing or wrong flow flowWeight configuration");
                            break;
                        }
                        case 'c':
                            std::getline(infile,lines);
                            break;
                        default:
                            throw new std::runtime_error("Unknown declaration.");
                    }
                    
                    if (mFlowWeights.size() == flowNum)// jump out the while loop
                        break;
                }
            }
            else
            {
                for (int i = 0;i < flowNum;++ i)
                    mFlowWeights.push_back(1.0);
            }
            
            // readmPackets
            while (!infile.eof() && infile >> c)
                switch(c)
                {
                    case 'p':// packet descriptions line
                    {
                        if (infile >> flowId >> packetId >> arrivalTime >> packetLength)
                        {
                            Packet *p = new Packet(flowId,packetId,packetLength,arrivalTime);
                            mPackets.push_back(p);
                        }
                        else throw new std::runtime_error("Missing or wrong packet description.");
                        std::getline(infile,lines);
                        break;
                    }
                    case 'c':// comments
                        std::getline(infile,lines);
                        break;
                    default:// unknown
                        throw new std::runtime_error("Unknown declaration.");
                }
                
                if (mPackets.empty())// no packet was found
                    throw new std::runtime_error("MissingmPackets description.");
                
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "Input Error:\n" << " " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Exception opening/reading file:\n" << "  " << e.what() << std::endl;
        }
        
        infile.close();
		mFlowLastDepartVTimes.resize(mFlowWeights.size());
        PKT_Compare_AT_L pc;
        std::sort(mPackets.begin(),mPackets.end(),pc);

  
        L_GPSsimulator = new L_GPSSim();

    }
    //! function to show all flows and packets
    void print()
    {
        std::cout << "===================================================================\n";
        std::cout << "                        Flow Information                           \n";
        std::cout << "===================================================================\n"; 
        std::cout << "flow number: " << mFlowWeights.size() << std::endl;
        int flowId = 1;
        for (auto f: mFlowWeights)
        {
            std::cout << "flow ID: " << flowId 
                      << ", flow flowWeight: " << f
                      << std::endl;
            ++ flowId;
        }

        std::cout << "===================================================================\n";
        std::cout << "\n\n";
        std::cout << "===================================================================\n";
        std::cout << "                        Packet Information                         \n";
        std::cout << "===================================================================\n";
        for (auto pkt: mPackets)
            std::cout << "arrival time: " << pkt->mArrivalTime 
                      << ", flow ID: " << pkt->mFlowId
                      << ", packet ID: " << pkt->mPacketId
                      << ", packet length: " << pkt->mLength
                      << std::endl;
        std::cout << "===================================================================\n";
    }
    void run()
    {
        int curPacketIndex = 0;// index of current packet
        Packet *pCurPacket = NULL;// pointer to current packet
        double weight = 0.0;
		double flowLastDepartVTime = 0.0;
        std::ofstream ofs("avl_tree.txt",std::ofstream::out);
        //! repeat until there are not packets
        while (curPacketIndex < mPackets.size())
        {
			pCurPacket = mPackets[curPacketIndex];
            weight = mFlowWeights[pCurPacket->mFlowId - 1];
			flowLastDepartVTime = mFlowLastDepartVTimes[pCurPacket->mFlowId - 1];
            pCurPacket->mGPS_VFTime = L_GPSsimulator->HandleNewPacketArrival(pCurPacket,weight,flowLastDepartVTime);
			mFlowLastDepartVTimes[pCurPacket->mFlowId - 1] = flowLastDepartVTime;
            //
            ofs << pCurPacket->mFlowId << " "
                << pCurPacket->mPacketId << " "
                << pCurPacket->mArrivalTime << " "
                << pCurPacket->mLength << std::endl;
            saveAVLTree2File(ofs);

            ++ curPacketIndex;
        }
        ofs.close();
        save2JSON();
    }
    void save2JSON()
    {
        json jDesp;
        json jFlow(mFlowWeights);
        jDesp["flow_weights"].push_back(jFlow);
        for (int i = 0;i < mPackets.size();++ i)
        {
            json jPacket = Packet2JSON(i);
            jDesp["packets"].push_back({jPacket});
        }
        std::string jString = jDesp.dump();
        std::cout << "\n\n";
        std::cout << "===================================================================\n";
        std::cout << "          Simulation results under GPS Simulator                   \n";
        std::cout << "===================================================================\n";
        std::cout << jString << std::endl;
        std::cout << "===================================================================\n";
        std::cout << "\n\n";
        std::cout << "Saving results to JSON file ...\n";
        std::ofstream ofs("gps_output.json", std::ofstream::out);
        ofs << jString << std::endl;
        ofs.close();




        std::cout << "Simulation finished!\n";
    }
    void saveAVLTree2File(std::ofstream& ofs)
    {
        //! print out the tree
        std::vector<DataField> treeData;
        std::vector<int> parents;
		std::vector<int> leftOrRight;
        L_GPSsimulator->GetAVLTree()->bfs(treeData,parents,leftOrRight);
        ofs << treeData.size() << std::endl;
        for (auto data: treeData)
        {
            ofs << data.mVTimeMax << " " 
                << data.mDeltaWeight << " "
                << data.mDeltaRTime << std::endl;
        }
        for (auto p: parents)
            ofs << p << " ";
        ofs << std::endl;
		for (auto lr : leftOrRight)
			ofs << lr << " ";
		ofs << std::endl;
        ofs << std::endl;
  
    }
};

#endif