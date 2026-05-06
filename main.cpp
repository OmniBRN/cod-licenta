#include <vector>
#include <utility>
#include <iostream>

#define uint unsigned int

class Node
{
    static uint id_cont;
    uint m_id;
public:
    Node(){
        m_id = id_cont;
        id_cont++;
    }
    uint get_id(){
        return m_id;
    }
};
uint Node::id_cont = 0;


class Lane
{
    double distance; 
};

class Edge
{
    Node nodes[2];
    Lane lanes;
};

class Network
{
    uint m_size;
    std::vector<std::vector<uint>> m_turn_map;
    std::vector<Node> m_nodes{};

public:
    Network(uint size, std::vector<std::pair<uint, std::vector<uint>>> connections){
        m_size = size;
        m_nodes.resize(size);
        m_turn_map.resize(size);
        for(std::pair p : connections){
            m_turn_map[p.first] = p.second;
        }
    }
    // Debug prints
    void print_node_ids()
    {
        for(Node node: m_nodes){
            std::cout << node.get_id() << ' ';
        }
        std::cout << '\n';
    }
    void print_all_connections()
    {
        for(uint i=0; i<m_size; i++)
        {
            std::cout << i << ": ";
            for(uint node_id : m_turn_map[i])
            {
                std::cout << node_id << ' ';
            }
            std::cout << '\n';
        }
    }
};



int main(){
    Network example = Network(4, {{0, {1,2,3}}, {1, {2, 3}}, {2, {1, 3}}, {3, {1, 2}}});
    example.print_node_ids();
    example.print_all_connections();
    return 0;
}