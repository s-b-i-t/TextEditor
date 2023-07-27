//  ECObserver.h

#ifndef ECOBERVER_H
#define ECOBERVER_H

#include <vector>
#include <algorithm>

class ECObserver
{
public:
    virtual ~ECObserver() {}
    virtual void Update() = 0;
};

class ECObserverSubject
{
public:
    ECObserverSubject() {}
    virtual ~ECObserverSubject() {}
    void Attach( ECObserver *pObs )
    {
        listObservers.push_back(pObs);
    }
    void Detach( ECObserver *pObs )
    {
        listObservers.erase(std::remove(listObservers.begin(), listObservers.end(), pObs), listObservers.end());
    }
    void Notify()
    {
        for(unsigned int i=0; i<listObservers.size(); ++i)
        {
            listObservers[i]->Update();
        }
    }
    
private:
    std::vector<ECObserver *> listObservers;
};

#endif
