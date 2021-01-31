#include <iostream>
#include <fstream>
#include "BSTree.h"
#include<vector>

using namespace std;

BSTree read_file(string file_name)
{
  fstream inFile;
  inFile.open(file_name);
  vector<int> inputVals;
    
  BSTree new_tree;
  ifstream infile(file_name); 

  int tmp;
  while(inFile >> tmp) {
    inputVals.push_back(tmp);
    new_tree.insert(tmp);
  }

  // print if n < 16
  if(inputVals.size() < 16) {
    cout << "Input: ";
    for(int i = 0; i < inputVals.size(); i++)
      cout << inputVals.at(i) << " ";
    cout << endl;
  }

  return new_tree;
}

void hline() {
  cout << "-------------------------" << endl << endl;
}

int main() {
  
  BSTree tree1;
  Node* test1 = tree1.insert(2);
  Node* test2 = tree1.insert(1);
  Node* test3 = tree1.insert(1);
  /*cout << "Test2 has location of: " << test2 << endl;
  cout << "Root has left child (test2) at " << test1->left << endl;
  cout << "Test2 has left child at " << test2->left << endl;*/
  cout << "Tree1 = " << endl << tree1 << endl;
  hline();

  // test the copy constructor
  BSTree tree2 = tree1;
  cout << "Tree2 (copy constructor) = " << endl << tree2 << endl;
  hline();

  // test the copy assignment
  BSTree tree3;
  tree3 = tree2;
  cout << "Tree3 (copy assignment) = " << endl << tree3 << endl;
  hline();

  // test the move constructor
  BSTree tree4 = move(tree2);
  cout << "Tree4 (move constructor from Tree2) = " << endl << tree4 << endl;
  cout << "Tree2 (after move constructor) = " << endl << tree2 << endl;
  hline();

  // test the move assignment
  BSTree tree5;
  tree5 = move(tree3);
  cout << "Tree5 (move assignment from Tree3) = " << endl << tree5 << endl;
  cout << "Tree3 (after move assignment) = " << endl << tree3 << endl;
  hline();

  for(int i = 1; i <= 12; i++) {
    string dir= "data-files/"+std::to_string(i);
    BSTree l = read_file(dir+"l");
    cout << "average search time linear " << i << " " << l.get_average_search_time() << endl;
    if(l.get_size() < 16) {
      cout << "Inorder traversal of linear ";
      l.inorder(cout);
    }
    cout << endl;
    ofstream out;
    out.open(dir+"l-Out");
    l.inorder(out);
    out.close();

    BSTree p = read_file(dir+"p");
    cout << "average search time perfect "<< i << " " << p.get_average_search_time() << endl;
    if(p.get_size() < 16) {
      cout << "Inorder traversal of prefect ";
      p.inorder(cout);
    }
    cout << endl;
    out.open(dir+"p-Out");
    l.inorder(out);
    out.close();

    BSTree r = read_file(dir+"r");
    cout << "average search time random "<< i << " " << r.get_average_search_time() << endl;
    if(r.get_size() < 16) {
      cout << "Inorder traversal of random ";
      r.inorder(cout);
    }
    cout << endl;
    out.open(dir+"r-Out");
    l.inorder(out);
    out.close();

    if(i <= 4)
      cout << "prefect tree " << i << endl << p;

    cout << endl;
  }
  
  hline();
  /*cout << "Tree1 = " << endl << tree1 << endl;
  //ostream& out;
  tree1.inorder(cout);*/
}

