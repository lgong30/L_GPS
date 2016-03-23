/*
	C++ Implementation for AVL tree.
	Shamelessly copy some parts from http://www.geeksforgeeks.org/avl-tree-set-2-deletion/ 
	version 1.0.0
	https://github.com/xlong88/bst

*/
#ifndef AVL_TREE_HPP
#define AVL_TREE_HPP


#include "bst.hpp"

//! A class for AVL Tree
/*!
 	Generic AVL Tree.
	Can be used with an customized comparator instead of the natural order,
	but the generic Value type must still be comparable.
*/
template <class T,class Compare = std::less<T> >
class AVL_Tree: public BST<T,Compare>
{
public:
	//! A constructor 
	AVL_Tree(Compare uLess = Compare()):BST<T,Compare>(uLess){}
	//! A constructor
	AVL_Tree(std::vector<T>& data,Compare uLess = Compare()){
		this->root = NULL;
		this->Less = uLess;
		for (auto d : data)
			insert(d);
		this->mSize = data.size();
	}
	//! A function to insert a new element in the AVL Tree
	void insert(T& data)
	{
		if (this->empty())
			this->root = new node<T>(data);
		else
			this->root = insert(this->root,data);
		++ this->mSize;
	}
	//! A recursive function to insert an element in the subtree rooted at current, perform rotation if necessary
	node<T>* insert(node<T> *current,T& data)
	{
		//! insert the new element
#ifdef AUGMENTED_L_GPS
		if (IsLeaf(current))
		{// reach leaf node
			if (Less(data,current->data))
			{
				current->right = new node<T>(current->data);
				current->left = new node<T>(data);
			}
			else if (Less(current->data,data))
			{
				current->left = new node<T>(current->data);
				current->right = new node<T>(data);
			}
			else
			{
				current->data.mDeltaWeight += data.mDeltaWeight;
			}
			current->height = std::max(height(current->left),height(current->right)) + 1;
			updateAugmentedMembers(current);
			return current;				
		}
#else
		if (current == NULL)
			return (new node<T>(data));
#endif

#ifdef AUGMENTED_L_GPS
		if (Less(data,current->data))
			current->left = insert(current->left,data);
		else if (Less(current->data,data))
			current->right = insert(current->right,data);
		else
		{
			current->data.mDeltaWeight += data.mDeltaWeight;
			return current;
		}		
#else
		if (Less(data,current->data))
			current->left = insert(current->left,data);
		else
			current->right = insert(current->right,data);
#endif
		//! update height
		current->height = std::max(height(current->left),height(current->right)) + 1;

#ifdef AUGMENTED_L_GPS
		updateAugmentedMembers(current);
#endif
		//! check whether rotation is needed
		int balance = heightDif(current);

		if (balance > 1)
		{//! left-heavy
			if (Less(data,current->left->data))
			{//! left-heavy
				return right_rotate(current);
			}
			else
			{//! right-heavy
				current->left = left_rotate(current->left);
				return right_rotate(current);
			}
		}
		else if (balance < -1)
		{//! right-heavy
			if (Less(data,current->right->data))
			{//!left-heavy
               current->right = right_rotate(current->right);
               return left_rotate(current);
			}
			else
			{//! right-heavy
				return left_rotate(current);
			}

		}
		return current;

	}
	//! A function to perform left rotate at current node
	node<T>* left_rotate(node<T>* current)
	{
		node<T>* right = current->right;
		current->right = right->left;
		right->left = current;

		current->height = std::max(height(current->left),height(current->right)) + 1;
		right->height = std::max(height(right->left),height(right->right)) + 1;

#ifdef AUGMENTED_L_GPS

		updateAugmentedMembers(current);
		updateAugmentedMembers(right);

#endif
		return right;
	}
#ifdef AUGMENTED_L_GPS
	inline void updateAugmentedMembers(node<T>* current)
	{
		if (IsLeaf(current)) return;
		current->data.mVTimeMax = current->right->data.mVTimeMax;
		current->data.mDeltaWeight = current->left->data.mDeltaWeight + current->right->data.mDeltaWeight;
		current->data.mDeltaRTime = current->left->data.mDeltaRTime + current->right->data.mDeltaRTime - (current->right->data.mVTimeMax - current->left->data.mVTimeMax) * current->left->data.mDeltaWeight;
	}
#endif
	//! A function to perform right rotate at current node
	node<T>* right_rotate(node<T>* current)
	{
		node<T>* left = current->left;
		current->left = left->right;
		left->right = current;

		current->height = std::max(height(current->left),height(current->right)) + 1;
		left->height = std::max(height(left->left),height(left->right)) + 1;

#ifdef AUGMENTED_L_GPS
		updateAugmentedMembers(current);
		updateAugmentedMembers(left);
#endif 
		return left;
	}
	int height()
	{
		return height(this->root);
	}
	//! A function to obtain the height of current node
	int height(node<T>* current)
	{
		if (current == NULL) return -1;
		return current->height;
	}
	//! A function to obtain the height difference between left and right child
	int heightDif(node<T>* current)
	{
		if (current == NULL) return 0;
		return height(current->left) - height(current->right);

	}
	//! A function to remove data in the AVL Tree
	void remove(T& data)
	{
		this->root = remove(this->root,data);
		-- this->mSize;
	}
#ifdef AUGMENTED_L_GPS
	//! Function to remove the leftmost leaf in the tree if it is no greater than data
	bool removeLeftmostLeafIfNecessary(T& data)
	{
		if (this->empty()) return false;
		if (this->root->left == NULL)
		{
			if (!Less(data, this->root->data))
			{
				data = this->root->data;
				this->root = this->root->right;
			}
			else
				return false;
			return true;
		}
		bool isRemoved = false;
		this->root = removeLeftmostLeafIfNecessary(this->root,data,isRemoved);
		return isRemoved;

	}
	//! Function to remove the leftmost leaf in the tree if it is no greater than data
	node<T>* removeLeftmostLeafIfNecessary(node<T>* current,T& data,bool& isRemoved)
	{
		if (IsLeaf(current->left))
		{
			if (!Less(data,current->left->data))
			{
				isRemoved = true;
				data = current->left->data;
				assert(current->right != NULL);
				current = current->right;
			}
			return current;
		}

		current->left = removeLeftmostLeafIfNecessary(current->left,data,isRemoved);

		current->height = std::max(height(current->left),height(current->right)) + 1;

		int balance = heightDif(current);

		updateAugmentedMembers(current);
		

		if (balance > 1)
		{// left-heavy
			if (heightDif(current->left) >= 0)
			{// left-heavy or balance
				return right_rotate(current);
			}
			else
			{// right-heavy
				current->left = left_rotate(current->left);
				return right_rotate(current);
			}

		}
		else if (balance < - 1)
		{// right-heavy
			if (heightDif(current->right) <= 0)
			{// right-heavy or balance
				return left_rotate(current);
			}
			else
			{// left-heavy
				current->right = right_rotate(current->right);
				return left_rotate(current);
			}
		}
        return current;


	}
#endif
	//! A function to remove element data from the subtree rooted at current, perform rotation if necessary
	node<T>* remove(node<T>* current,T& data)
	{
		if (current == NULL)
			throw new std::runtime_error("Cannot remove non-exist element");

		if (data == current->data)
		{
			if (!current->left)
				return remove(current->right,data);
			else if (!current->right)
				return remove(current->left,data);
			else
			{
				current->data = retrievalData(current->left);
				current->left = remove(current->left,current->data);
			}
		}

		if (current == NULL) return current;

		current->height = std::max(height(current->left),height(current->right)) + 1;

		int balance = heightDif(current);

#ifdef AUGMENTED_L_GPS

		updateAugmentedMembers(current);
		updateAugmentedMembers(right);

#endif

		if (balance > 1)
		{// left-heavy
			if (heightDif(current->left) >= 0)
			{// left-heavy or balance
				return right_rotate(current);
			}
			else
			{// right-heavy
				current->left = left_rotate(current->left);
				return right_rotate(current);
			}

		}
		else if (balance < - 1)
		{// right-heavy
			if (heightDif(current->right) <= 0)
			{// right-heavy or balance
				return left_rotate(current);
			}
			else
			{// left-heavy
				current->right = right_rotate(current->right);
				return left_rotate(current);
			}
		}
        return current;

	}
	//! Function to return whether a specific node is leaf or node
	bool IsLeaf(node<T>* current)
	{
		return (current->left == NULL && current->right == NULL);
	}
	node<T>* GetRoot()
	{
		return this->root;
	}
	bool IsAVLTree()
	{
		if (this->empty()) return true;
		return IsAVLTree(this->root);
	}
	bool IsAVLTree(node<T>* current)
	{
		if (current == NULL) return true;
		if (abs(heightDif(current)) > 1) return false;
		return IsAVLTree(current->left) && IsAVLTree(current->right);
	}
};

#endif