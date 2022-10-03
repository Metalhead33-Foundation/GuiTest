#ifndef MHBINARYTREE_HPP
#define MHBINARYTREE_HPP
#include <memory>
#include <functional>
namespace MH33 {
namespace Interface {

template <typename T> struct BinaryTreeNode {
	typedef std::unique_ptr<BinaryTreeNode> Leaf;
	typedef std::function<void(T&)> TraversalFunction;
	typedef std::function<void(const T&)> TraversalConstFunction;
	typedef std::function<void(T&,bool&)> StoppableTraversalFunction;
	typedef std::function<void(const T&,bool&)> StoppableTraversalConstFunction;
	Leaf left;
	Leaf right;
	T data;
	void inorder(const TraversalFunction& func) {
		if(left) left->inorder(func);
		func(data);
		if(right) right->inorder(func);
	}
	void inorder(const TraversalConstFunction& func) const {
		if(left) left->inorder(func);
		func(data);
		if(right) right->inorder(func);
	}
	void inorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(left) left->inorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		if(right) right->inorder(func,shouldStop);
		}
	}
	void inorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(left) left->inorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		if(right) right->inorder(func,shouldStop);
		}
	}
	void preorder(const TraversalFunction& func) {
		func(data);
		if(left) left->preorder(func);
		if(right) right->preorder(func);
	}
	void preorder(const TraversalConstFunction& func) const {
		func(data);
		if(left) left->preorder(func);
		if(right) right->preorder(func);
	}
	void preorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(!shouldStop) {
		func(data,shouldStop);
		if(left) left->preorder(func,shouldStop);
		if(right) right->preorder(func,shouldStop);
		}
	}
	void preorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(!shouldStop) {
		func(data,shouldStop);
		if(left) left->preorder(func,shouldStop);
		if(right) right->preorder(func,shouldStop);
		}
	}
	void postorder(const TraversalFunction& func) {
		if(left) left->postorder(func);
		if(right) right->postorder(func);
		func(data);
	}
	void postorder(const TraversalConstFunction& func) const {
		if(left) left->postorder(func);
		if(right) right->postorder(func);
		func(data);
	}
	void postorder(const StoppableTraversalFunction& func, bool& shouldStop) {
		if(left) left->postorder(func,shouldStop);
		if(right) right->postorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		}
	}
	void postorder(const StoppableTraversalConstFunction& func, bool& shouldStop) const {
		if(left) left->postorder(func,shouldStop);
		if(right) right->postorder(func,shouldStop);
		if(!shouldStop) {
		func(data,shouldStop);
		}
	}
	size_t subtreeSize() const {
		size_t i=0;
		preorder([&i](T& dat) { void(dat); ++i; } );
		return i;
	}
	size_t childCount() const { return subtreeSize() - 1; }
	T& minimum() {
		T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr >= dat) ptr = &dat;
		} );
		return *ptr;
	}
	const T& minimum() const {
		const T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr >= dat) ptr = &dat;
		} );
		return *ptr;
	}
	T& maximum() {
		T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr <= dat) ptr = &dat;
		} );
		return *ptr;
	}
	const T& maximum() const {
		const T* ptr = &data;
		preorder([&ptr](T& dat) {
			if(*ptr <= dat) ptr = &dat;
		} );
		return *ptr;
	}
};
template <typename T> struct BinaryTree {
	typedef BinaryTreeNode<T> Node;
	typedef Node::TraversalFunction TraversalFunction;
	typedef Node::TraversalConstFunction TraversalConstFunction;
	typedef Node::StoppableTraversalFunction StoppableTraversalFunction;
	typedef Node::StoppableTraversalConstFunction StoppableTraversalConstFunction;
	Node root;
	// Inorder traversal
	void inorder(const TraversalFunction& func) {
		root.inorder(func);
	}
	void inorder(const TraversalConstFunction& func) const {
		root.inorder(func);
	}
	void inorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.inorder(func,shouldStop);
	}
	void inorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.inorder(func,shouldStop);
	}
	// Preorder traversal
	void preorder(const TraversalFunction& func) {
		root.preorder(func);
	}
	void preorder(const TraversalConstFunction& func) const {
		root.preorder(func);
	}
	void preorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.preorder(func,shouldStop);
	}
	void preorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.preorder(func,shouldStop);
	}
	// Postorder traversal
	void postorder(const TraversalFunction& func) {
		root.postorder(func);
	}
	void postorder(const TraversalConstFunction& func) const {
		root.postorder(func);
	}
	void postorder(const StoppableTraversalFunction& func) {
		bool shouldStop = false;
		root.postorder(func,shouldStop);
	}
	void postorder(const StoppableTraversalConstFunction& func) const {
		bool shouldStop = false;
		root.postorder(func,shouldStop);
	}
	// Min-max
	size_t size() const { return root.subtreeSize(); }
	T& minimum() { return root.minimum(); }
	const T& minimum() const { return root.minimum(); }
	T& maximum() { return root.maximum(); }
	const T& maximum() const { return root.maximum(); }
};

}
}
#endif // MHBINARYTREE_HPP
