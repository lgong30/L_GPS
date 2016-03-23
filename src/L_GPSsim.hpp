/*
   A C++ implementation of Exact GPS simulator with logarithmic complexity. The underlying idea
   is from an academic paper published in IEEE/ACM Transactions on Networking (its citation will
   be presented in the following part), however, the implementation details are not the same.
   
   For more details about the underlying idea, you can refer to the following paper:
   Valente, P., 2007. Exact GPS simulation and optimal fair scheduling with 
   logarithmic complexity. Networking, IEEE/ACM Transactions on, 15(6), pp.1454-1466.

   The following gives the abstract of the above paper:
   Generalized processor sharing (GPS) is a fluid scheduling policy providing perfect 
   fairness over both constant-rate and variable-rate links. The minimum deviation 
   (lead/lag) with respect to the GPS service achievable by a packet scheduler is one 
   maximum packet size. To the best of our knowledge, the only packet scheduler 
   guaranteeing the minimum deviation is worst-case fair weighted fair queueing, 
   which requires on-line GPS simulation. Existing algorithms to perform GPS simulation 
   have worst-case computational complexity per packet transmission (being the number of 
   competing flows). Hence, has been charged for complexity too. However it has been 
   proven that the lower bound complexity to guarantee deviation is, yet a scheduler 
   achieving such a result has remained elusive so far. In this paper, we present L-GPS, 
   an algorithm that performs exact GPS simulation with worst-case complexity and small 
   constants. As such it improves the complexity of all the packet schedulers based on 
   GPS simulation. We also present , an implementation of based on L-GPS. has complexity 
   with small constants, and, since it achieves the minimum possible deviation, it does 
   match the aforementioned complexity lower bound. Furthermore, both L-GPS and comply 
   with constant-rate as well as variable-rate links. We assess the effectiveness of both 
   algorithms by simulating real-world scenarios.

   Note that, if you find any bugs in this implementation or have any improvement suggestions,
   please feel free to send email to me (gonglong.gatech@gmail.com) or leave comments on the
   corresponding blog post (http://xlong88.github.io/L-GPS-Implementation).
*/

#ifndef L_GPS_HPP
#define L_GPS_HPP

#define AUGMENTED_L_GPS

#include "avlTree.hpp"
#include "packet.hpp"

//! class for data of the node in AVL tree
class DataField{
public:
	double mVTimeMax;
	double mDeltaWeight;
	double mDeltaRTime;
	DataField()
	{
		mVTimeMax = 0.0;
		mDeltaRTime = 0.0;
		mDeltaWeight = 0.0;
	}
	DataField(double VTime,double deltaWeight)
	{
		mVTimeMax = VTime;
		mDeltaWeight = deltaWeight;
		mDeltaRTime = 0.0;
	}

};

//! compare class based on DataField's mVTimeMax
class Compare_VTM_L { // simple comparison function
   public:
      bool operator()(const DataField& d1,const DataField& d2) { return d1.mVTimeMax < d2.mVTimeMax; } 
};

class L_GPSSim{
	//! old value for virtual time 
	double mOldVTime;
	//! old value for real time 
	/*! actually, it should be the amount of service, 
	    since this implementation assumes service rate is 1,
	    therefore, it equals to the real time)
	*/
	double mOldRTime;
	//! old value for total weight of all the flows at time (mOldRTime)^+
	double mSumWeight;

	//! balanced tree 
	/*! the balanced tree stores all the break points and expected
	break points after time mOldRTime
	*/
	AVL_Tree<DataField, Compare_VTM_L> *mpBalancedTree;
public:
	//! constructor
	L_GPSSim()
	{
		mOldRTime = 0;
		mOldRTime = 0;
		mSumWeight = 0;

		mpBalancedTree = new AVL_Tree<DataField, Compare_VTM_L>();
	}
	//! function to handle the event of packet arrival 
	/*! note that upon each packet arrival event, there are at most two updates
	    are triggered, the first one is corresponding to the arrival time (VTime)
	    of the newly arriving packet (it creates a break point if the flow is belongs
	    to is inactive when it arrives, or it might cancel the expected break point 
	    creating by its previous packet in the same flow); the second is corresponding 
	    to the finish time (VTime) of this packet, because it is an expected break point.

	    For the definitions of all the concepts that mentioned above, you can refer to 
	    the paper:
	   	Valente, P., 2007. Exact GPS simulation and optimal fair scheduling with 
	   	logarithmic complexity. Networking, IEEE/ACM Transactions on, 15(6), pp.1454-1466.
	*/
	double HandleNewPacketArrival(Packet* pPKT,double flowWeight,double& flowLastDepartVTime)
	{
		/*! get the three important parameters related to this newly arriving
            packet: real time (arrival time), packet length (in terms of bytes),
            and weight of the flow this packet belongs to
        */
		double newRTime = pPKT->mArrivalTime;
		double packetLength = pPKT->mLength;
		//double eps = 1e-8;

		/*! calculate the virtual start time and virtual finish time of this
			packet (details you can refer to the description of the function
			RTime2VTime())
		*/
		double curVTime = RTime2VTime(newRTime);
		double newVTime = curVTime;
		if (newVTime < flowLastDepartVTime)
			newVTime = flowLastDepartVTime;
		double newExpectedBreakPoint = newVTime + packetLength / flowWeight;
		flowLastDepartVTime = newExpectedBreakPoint;
		//! insert the "break point" corresponding to the arrival of this packet
		//if (! mpBalancedTree->empty() && abs(newVTime) > eps)
		Append(curVTime,newVTime,flowWeight);
		/*! insert the expected break point corresponding to the departure of this 
		    packet
		*/
		Append(curVTime,newExpectedBreakPoint,-flowWeight);

		return newExpectedBreakPoint;
	}
	//! Function to compute the corresponding virtual time for a new real time
	/*!
		this function performs a binary search for the NewRTime on all the break points
		and expected points in the balanced binary search tree (i.e., AVL tree in this 
		implementation)

		for more details on this calculation, you can refer to the paper:
	   	Valente, P., 2007. Exact GPS simulation and optimal fair scheduling with 
	   	logarithmic complexity. Networking, IEEE/ACM Transactions on, 15(6), pp.1454-1466.
	*/
	double RTime2VTime(double NewRTime)
	{
		//! virtual time, real time and total weight after last event
		double oldVTime = mOldVTime,
			   oldRTime = mOldRTime,
			   oldSumWeight = mSumWeight;
		double eps = 1e-8;

	    //! obtain the root of the AVL tree
		node<DataField> *pCurNode = mpBalancedTree->GetRoot();
		if (pCurNode != NULL && abs(oldSumWeight) > eps)
		{
			//! perform search on the tree
			while (!mpBalancedTree->IsLeaf(pCurNode))
			{
				double RTimeLMax = oldRTime + (pCurNode->left->data.mVTimeMax - oldVTime) * oldSumWeight - pCurNode->left->data.mDeltaRTime;

				if (NewRTime < RTimeLMax) //! locate in left subtree
					pCurNode = pCurNode->left;
				else//! locate in the right subtree
				{
					oldSumWeight += pCurNode->left->data.mDeltaWeight;
					
					oldVTime = pCurNode->left->data.mVTimeMax;
					oldRTime = RTimeLMax;

					pCurNode = pCurNode->right;
				}
			}
			return oldVTime + (NewRTime - oldRTime) / oldSumWeight;
		}

			return 0.0;	
	}
	//! function to insert a node (i.e., a break point or an expected break point)
	/*! this function insert a new node into the AVL tree, and it calls the function
		RemoveBreakPointIfNecessary() to remove the leftmost left node in the tree if
		necessary.
		1. note that, the insertion is slightly different from the standard insertion 
		procedure in the binary search tree, it guarantees that the parent of the 
		inserted node would have two children (more details you can refer to the description
		of the insert() function of the AVL tree)
		2. note that, the implementation of this function is not the same as the one 
		described in the paper, but the underlying idea is the same
	*/
	void Append(double curVTime,double newVTime,double newDeltaWeight)
	{
		//! create the data field 
		DataField data(newVTime,newDeltaWeight);
		//! perform insertion, data field updates of affected nodes and re-balance if necessary
		mpBalancedTree->insert(data);
		//! perform removal when necessary
		RemoveBreakPointIfNecessary(curVTime);
	}
	//! function to remove the leftmost leaf in the tree when necessary
	/*! this function removes the leftmost leaf in the AVL tree when its mVTimeMax is no 
		greater than the current virtual time (i.e., curVTime) and it will update the 
		members: mOldRTime, mOldVTime, and mSumWeight.

		1. note that, the removal procedure is slightly different from the standard removal 
		operation of the binary search tree, if will replace the parent of the removed 
		node by its sibling (i.e., the right child of its parent) 
		2. note that, the implementation is not the same as the one described in the paper, 
		but the underlying idea is the same.
	*/
	void RemoveBreakPointIfNecessary(double curVTime)
	{
		//! create the data field 
		DataField data(curVTime,0);
		//! remove the leftmost leaf when necessary
		if (mpBalancedTree->removeLeftmostLeafIfNecessary(data))
		{
			//! update all the member variables
			mOldRTime += mSumWeight * (data.mVTimeMax - mOldVTime); //! TODO: check its correctness
			mOldVTime = data.mVTimeMax;
			mSumWeight += data.mDeltaWeight;
		}
	}
	AVL_Tree<DataField,Compare_VTM_L>* GetAVLTree()
	{
		return mpBalancedTree;
	}

};

#endif