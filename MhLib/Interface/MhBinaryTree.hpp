#ifndef MHBINARYTREE_HPP
#define MHBINARYTREE_HPP
#include <memory>
#include <functional>
namespace MH33 {
namespace Interface {

/*! A binary tree node.*/
/*!
  \tparam T The type of element contained within the binary tree node.
*/
template <typename T> struct BinaryTreeNode {
	/*! A leaf in the binary tree node.*/
	typedef std::unique_ptr<BinaryTreeNode> Leaf;
	/*! A traversal function*/
	typedef std::function<void(T&)> TraversalFunction;
	/*! A traversal function*/
	typedef std::function<void(const T&)> TraversalConstFunction;
	/*! A traversal function. Second parameter is a boolean about whether the function should stop at that point or not.*/
	typedef std::function<void(T&,bool&)> StoppableTraversalFunction;
	/*! A traversal function. Second parameter is a boolean about whether the function should stop at that point or not.*/
	typedef std::function<void(const T&,bool&)> StoppableTraversalConstFunction;
	/*! Left leaf.*/
	Leaf left;
	/*! Right leaf.*/
	Leaf right;
	/*! Data contained within the binary tree node.*/
	T data;
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const TraversalFunction& func) {
		if(left) left->inorder(func);
		func(data);
		if(right) right->inorder(func);
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const TraversalConstFunction& func) const {
		if(left) left->inorder(func);
		func(data);
		if(right) right->inorder(func);
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void inorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(left) left->inorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		if(right) right->inorder(func,shouldStop);
		}
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void inorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(left) left->inorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		if(right) right->inorder(func,shouldStop);
		}
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const TraversalFunction& func) {
		func(data);
		if(left) left->preorder(func);
		if(right) right->preorder(func);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const TraversalConstFunction& func) const {
		func(data);
		if(left) left->preorder(func);
		if(right) right->preorder(func);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void preorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(!shouldStop) {
		func(data,shouldStop);
		if(left) left->preorder(func,shouldStop);
		if(right) right->preorder(func,shouldStop);
		}
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void preorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(!shouldStop) {
		func(data,shouldStop);
		if(left) left->preorder(func,shouldStop);
		if(right) right->preorder(func,shouldStop);
		}
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const TraversalFunction& func) {
		if(left) left->postorder(func);
		if(right) right->postorder(func);
		func(data);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const TraversalConstFunction& func) const {
		if(left) left->postorder(func);
		if(right) right->postorder(func);
		func(data);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void postorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(left) left->postorder(func,shouldStop);
		if(right) right->postorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		}
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	  \param shouldStop An lvalue reference to a boolean value that may be modified by the traversal function. When set to true, this function stops.
	*/
	void postorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(left) left->postorder(func,shouldStop);
		if(right) right->postorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		}
	}
	//! Returns the amount of nodes within the subtree, including the current node.
	/*!
	  \return The amount of nodes within the subtree, including the current node.
	*/
	size_t subtreeSize() const {
		size_t i=0;
		preorder([&i](T& dat) { (void)dat; ++i; } );
		return i;
	}
	//! Returns the amount of nodes within the subtree, excluding the current node.
	/*!
	  \return The amount of nodes within the subtree, excluding the current node.
	*/
	size_t childCount() const { return subtreeSize() - 1; }
	//! Returns a reference to the "minimum" element of the subtree, which also includes the current node.
	/*!
	  \return A reference to the "minimum" element of the subtree, which also includes the current node.
	*/
	T& minimum() {
		T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr >= dat) ptr = &dat;
		} );
		return *ptr;
	}
	//! Returns a const reference to the "minimum" element of the subtree, which also includes the current node.
	/*!
	  \return A const reference to the "minimum" element of the subtree, which also includes the current node.
	*/
	const T& minimum() const {
		const T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr >= dat) ptr = &dat;
		} );
		return *ptr;
	}
	//! Returns a reference to the "maximum" element of the subtree, which also includes the current node.
	/*!
	  \return A reference to the "maximum" element of the subtree, which also includes the current node.
	*/
	T& maximum() {
		T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr <= dat) ptr = &dat;
		} );
		return *ptr;
	}
	//! Returns a const reference to the "maximum" element of the subtree, which also includes the current node.
	/*!
	  \return A const reference to the "maximum" element of the subtree, which also includes the current node.
	*/
	const T& maximum() const {
		const T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr <= dat) ptr = &dat;
		} );
		return *ptr;
	}
};
/*! A binary tree.*/
/*!
  \tparam T The type of element contained within the nodes of the binary tree.
*/
template <typename T> struct BinaryTree {
	/*! A node within the binary tree.*/
	typedef BinaryTreeNode<T> Node;
	/*! A traversal function.*/
	typedef typename Node::TraversalFunction TraversalFunction;
	/*! A traversal function.*/
	typedef typename Node::TraversalConstFunction TraversalConstFunction;
	/*! A traversal function.*/
	typedef typename Node::StoppableTraversalFunction StoppableTraversalFunction;
	/*! A traversal function.*/
	typedef typename Node::StoppableTraversalConstFunction StoppableTraversalConstFunction;
	/*! The root node within the binary tree.*/
	Node root;
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const TraversalFunction& func) {
		root.inorder(func);
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const TraversalConstFunction& func) const {
		root.inorder(func);
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.inorder(func,shouldStop);
	}
	//! In-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Visit the current node, calling FUNC on it.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void inorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.inorder(func,shouldStop);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const TraversalFunction& func) {
		root.preorder(func);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const TraversalConstFunction& func) const {
		root.preorder(func);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.preorder(func,shouldStop);
	}
	//! Pre-order traversal.\n
	//! 1. Visit the current node, calling FUNC on it.\n
	//! 2. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 3. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void preorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.preorder(func,shouldStop);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const TraversalFunction& func) {
		root.postorder(func);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const TraversalConstFunction& func) const {
		root.postorder(func);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.postorder(func,shouldStop);
	}
	//! Post-order traversal.\n
	//! 1. Recursively traverse the current node's left subtree by calling inorder() on the left leaf, if it exists.\n
	//! 2. Recursively traverse the current node's right subtree by calling inorder() on the right leaf, if it exists.\n
	//! 3. Visit the current node, calling FUNC on it.\n
	/*!
	  \param func The functioned that is called on every node that is visited during the traversal.
	*/
	void postorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.postorder(func,shouldStop);
	}
	//! Returns the amount of nodes in the tree.
	/*!
	  \return The amount of nodes in the tree.
	*/
	size_t size() const { return root.subtreeSize(); }
	//! Returns a const reference to the "minimum" element of the tree.
	/*!
	  \return A const reference to the "minimum" element of the tree.
	*/
	T& minimum() { return root.minimum(); }
	//! Returns a const reference to the "minimum" element of the tree.
	/*!
	  \return A const reference to the "minimum" element of the tree.
	*/
	const T& minimum() const { return root.minimum(); }
	//! Returns a const reference to the "maximum" element of the tree.
	/*!
	  \return A const reference to the "maximum" element of the tree.
	*/
	T& maximum() { return root.maximum(); }
	//! Returns a const reference to the "maximum" element of the tree.
	/*!
	  \return A const reference to the "maximum" element of the tree.
	*/
	const T& maximum() const { return root.maximum(); }
};

}
}
#endif // MHBINARYTREE_HPP
