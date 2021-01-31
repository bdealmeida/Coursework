#include "BSTree.h"

using namespace std;

// input / output operators
ostream& operator<<(ostream& out, BSTree& tree) {
  tree.print_level_by_level(out);
  return out;
}

ostream& operator<<(ostream& out, Node node) {
  return out << "( " << node.value << ", "
	     << node.search_time << " )";
}

istream& operator>>(istream& in, BSTree& tree) {
  /*
    take input from the in stream, and build your tree
    input will look like
    4 
    2 
    6 
    1 
    3 
    5 
    7
  */
  int next;
  while(in >> next) 
    tree.insert(next);
  return in;
}

///////////// Implementation To Do ////////////////////

// copy constructor
BSTree::BSTree(const BSTree& other) {
  root = deepCopy(other.root);
  size = other.size;
}

// move constructor
BSTree::BSTree(BSTree&& other) {
  if(this == &other)
    return;

  root = other.root;
  size = other.size;

  other.root = nullptr;
  other.size = 0;
}

//copy assignment
BSTree& BSTree::operator=(const BSTree& other) {
  if(this == &other)
    return *this;
  
  // Remove previous tree
  if(root != nullptr)
    deallocateTree(root);

  // Deep copy 'other'
  root = deepCopy(other.root);
}

// move assignment
BSTree BSTree::operator=(BSTree&& other) {
  root = other.root;
  size = other.size;

  other.root = nullptr;
  other.size = 0;

  return *this;
}

// destructor
BSTree::~BSTree() {
  deallocateTree(root);
  size = 0;
}

// Recursive deep copy
inline Node* BSTree::deepCopy(Node* cur) {
  if(cur == nullptr)
    return cur;

  // Copy down left and right sides
  Node* newLeft = deepCopy(cur->left);
  Node* newRight = deepCopy(cur->right);

  // Build new node
  Node* newNode = new Node(cur->value, newLeft, newRight, cur->search_time);
  return newNode;
}

// Recursive
inline void BSTree::deallocateTree(Node* cur) {
  if(cur == nullptr)
    return;
  //cout << "Deallocating node (" << cur << ") with:\n Left: " << cur->left << "\nRight: " << cur->right << endl;

  deallocateTree(cur->left);
  cur->left = nullptr;
  deallocateTree(cur->right);
  cur->right = nullptr;
  
  //cout << "Children cleared... clearing other values." << endl;
  cur->value = 0;
  cur->search_time = 0;
}

// recursive
void BSTree::copy_helper(Node* copy_to, const Node* copy_from) const {
  if(copy_from == nullptr)
    return;

  if(copy_from->left != nullptr) {
    copy_to->left = new Node(*copy_from->left);
    copy_helper(copy_to->left, copy_from->left);
  } else {
    copy_to->left = nullptr;
  }

  if(copy_from->right != nullptr) {
    copy_to->right = new Node(*copy_from->right);
    copy_helper(copy_to->right, copy_from->right);
  } else {
    copy_to->right = nullptr;
  }
}


Node* BSTree::insert(int obj) {
  if(size == 0) {
    root = new Node(obj, nullptr, nullptr, 1);
    size++;
    return root;
  }

  Node* cur = root;
  bool inserting = true;
  Node* newNode;

  // "Recursively" travel to first empty slot where node fits, insert new node
  while(inserting) {
    if(obj <= cur->value) {
      if(cur->left == nullptr) {
        newNode = new Node(obj, nullptr, nullptr, cur->search_time + 1);
        cur->left = newNode;
        inserting = false;
      } else
        cur = cur->left;
    } else { // obj > cur->val
      if(cur->right == nullptr) {
        newNode = new Node(obj, nullptr, nullptr, cur->search_time + 1);
        cur->right = newNode;
        inserting = false;
      } else
        cur = cur->right;
    }
  }

  size++;
  return newNode;
}

Node* BSTree::search(int obj) {
  if(size == 0)
    return nullptr;

  Node* cur = root;

  while(true)
    if(obj == cur->value)
      return cur;
    else if(obj < cur->value)
      if(cur->left == nullptr)
        return nullptr;
      else
        cur = cur->left;
    else // obj > cur->val
      if(cur->right == nullptr)
        return nullptr;
      else
        cur = cur->right;
}

void BSTree::update_search_times() {
  if(root == nullptr)
    return;

  root->search_time = 1;
  update_search_times(root);
}

inline void BSTree::update_search_times(Node* root) {
  if(root->left != nullptr)
    root->left->search_time = root->search_time+1;
  if(root->right != nullptr)
    root->right->search_time = root->search_time+1;
  
  update_search_times(root->left);
  update_search_times(root->right);
}


ostream& BSTree::inorder(ostream& out) {
  /*
    print your nodes in infix order
    if our tree looks like 

        4
      2   6
     1 3 5 7

    we should get

    1 2 3 4 5 6 7 
  */
  printInOrder(out, root);
  out << endl;
  return out;
}

ostream& BSTree::printInOrder(ostream& out, Node* root) {
  if(root->left != nullptr)
    printInOrder(out, root->left); // Print left subtree first
  
  pretty_print_node(out, root); // Print root

  if(root->right != nullptr) // Print right subtree last
    printInOrder(out, root->right);

  return out;
}

// implemented
void pretty_print_node(ostream& out, Node* node) {
  out << node->value << "["<<node->search_time<<"] ";
}

// implemented
ostream& BSTree::print_level_by_level(ostream& out) {
  /*
    print the tree using a BFS 
    output should look like this if we dont have a full tree

    4
    2 6
    1 X 5 7
    X X X X X X X 9

    it will be helpfull to do this part iterativly, 
    so do the BFS with the std stack data structure.

    it may also be helpfull to put the entire tree into a vector 
    (probably google this if you dont know how to do it)
  */

  if(root == nullptr)
    return out;

  bool hasNodes = true;
  vector<Node*> current_level;
  vector<Node*> next_level;
  current_level.push_back(root);
  while(hasNodes) {
    hasNodes = false;
    for(auto node : current_level) {
      if(node != nullptr) {
	pretty_print_node(out, node);
	if(node->left != nullptr)
	  hasNodes = true;
	if(node->right != nullptr)
	  hasNodes = true;
	
	next_level.push_back(node->left);
	next_level.push_back(node->right);
      } else {
	out << "X ";
	next_level.push_back(nullptr);
	next_level.push_back(nullptr);
      }
    }
    out << endl;
    current_level.clear();
    current_level = next_level;
    next_level.clear();
  }

  return out;
}

// implemented
int get_total_search_time(Node* node) {
  if(node == nullptr)
    return 0;

  return get_total_search_time(node->left) + get_total_search_time(node->right) + node->search_time;
}

// implemented
float BSTree::get_average_search_time() {
  int total_search_time = get_total_search_time(root);
  if(total_search_time == 0)
    return -1;
	
  return ((float)total_search_time)/size;
}