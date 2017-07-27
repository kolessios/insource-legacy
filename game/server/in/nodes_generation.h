//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NODES_GENERATION_H
#define NODES_GENERATION_H

#ifdef _WIN32
#pragma once
#endif

class CNodeEnt;

//================================================================================
//================================================================================
class CNodesGeneration
{
public:
    virtual void Start();

    virtual void GenerateWalkableNodes();
    virtual void GenerateClimbNodes();
    virtual void GenerateHintNodes();

protected:
    int m_iNavAreaCount;
    int m_iWalkableNodesCount;

    CUtlVector<Vector> m_WalkLocations;
};

#endif // NODES_GENERATION_H