#include <ostream>

using namespace std;

struct employee {
    int id;
    char name[10];
    double hours;

    void print(ostream &out){
        out << "id: " << id
            << "\tname: " << name
            << "\thours: " << hours << endl;
    }
};

int employeeComparator(const void* a, const void* b){
    return ((employee*)a)->id - ((employee*)b)->id;
}