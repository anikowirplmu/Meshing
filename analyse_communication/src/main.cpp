#include <iostream>
#include <set>
#include <cstring>
#include <utils/args.h>

#include "common/PartitionReader.h"
#include "Graph.h"

void printCounter(std::string const& name, counter_t* counter, unsigned numPartitions)
{
  std::cout << std::endl << name;
  for (unsigned p = 0; p < numPartitions; ++p) {
    if (p % 10 == 0) {
      std::cout << std::endl;
    }
    std::cout << counter[p] << " ";
  }
  std::cout << std::endl << std::endl;
}

int main(int argc, char** argv)
{
  utils::Args args;
  args.addOption("dot", 'd', "Write dot file for visualisation.", utils::Args::Required, false);
  args.addOption("matrix", 'm', "Write edge-cut matrix.", utils::Args::Required, false);
  args.addAdditionalOption("mesh", "Mesh file");

  if (args.parse(argc, argv) != utils::Args::Success) {
    return -1;
  }

  std::string meshFile = args.getAdditionalArgument<std::string>("mesh");
  std::string dotFile = args.getArgument<std::string>("dot", "");
  std::string matrixFile = args.getArgument<std::string>("matrix", "");

  PartitionReader reader(meshFile);
  
  counter_t totalEdgeCut = 0;
  counter_t totalCommVolume = 0;
  
  counter_t* edgeCut = new counter_t[reader.partitions];
  counter_t* commVolume = new counter_t[reader.partitions];
  
  memset(edgeCut, 0, reader.partitions*sizeof(counter_t));
  memset(commVolume, 0, reader.partitions*sizeof(counter_t));
  
  Graph edgeCutGraph(reader.partitions);

  for (unsigned p = 0; p < reader.partitions; ++p) {
    if (p%10 == 0) {
      std::cout << "Reading partition " << p << std::endl;
    }
    
    reader.readPartition(p);
    
    for (unsigned elem = 0; elem < reader.elementSize[p]; ++elem) {
      for (unsigned face = 0; face < 4; ++face) {
        int neighborRank = reader.elementNeighborRanks[4*elem + face];
        if (neighborRank != p) {
          ++edgeCut[p];
          edgeCutGraph.addEdge(p, static_cast<unsigned>(neighborRank));
        }
      }
      std::set<int> neighbors(reader.elementNeighborRanks + 4*elem, reader.elementNeighborRanks + 4*elem + 4);
      neighbors.erase(p);
      commVolume[p] += neighbors.size();
    }
    
    totalEdgeCut += edgeCut[p];
    totalCommVolume += commVolume[p];
  }
  
  if (dotFile.size() > 0) {
    edgeCutGraph.printDOT(dotFile);
  }
  if (matrixFile.size() > 0) {
    edgeCutGraph.printMatrix(matrixFile);
  }
  
  printCounter("Edge cut", edgeCut, reader.partitions);
  printCounter("Communication volume", commVolume, reader.partitions);
  
  std::cout << "Total edge cut: " << totalEdgeCut << std::endl;
  std::cout << "Total communication volume: " << totalCommVolume << std::endl;
  std::cout << "Ratio: " << static_cast<double>(totalEdgeCut) / totalCommVolume << std::endl << std::endl;
  
  std::cout << "Order\t\tEdge cut (MB)\tComm volume (MB)" << std::endl;
  for (unsigned order = 1; order <= 8; ++order) {
    std::cout << order << "\t\t" << sizeof(double)*9*order*(order+1)/2.0 * totalEdgeCut / (1024.0*1024.0)
                       << "\t\t" << sizeof(double)*9*order*(order+1)*(order+2)/6.0 * totalCommVolume / (1024.0*1024.0) << std::endl;
  }
  
  delete[] edgeCut;
  delete[] commVolume;
  
  return 0;
}
