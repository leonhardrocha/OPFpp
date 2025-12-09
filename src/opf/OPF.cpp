/*
  Copyright (C) <2009> <Alexandre Xavier Falcão and João Paulo Papa>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  please see full copyright in COPYING file.
  -------------------------------------------------------------------------
  written by A.X. Falcão <afalcao@ic.unicamp.br> and by J.P. Papa
  <papa.joaopaulo@gmail.com>, Oct 20th 2008

  This program is a collection of functions to manage the Optimum-Path Forest (OPF)
  classifier.*/

#include "opf.hpp"
#include "opf/common.hpp"
#include "opf/Subgraph.hpp"
#include "opf/RealHeap.hpp"
#include "opf/set.hpp"


char	opf_PrecomputedDistance;
float  **opf_DistanceValue;

using namespace opf;

Subgraph::DistanceFunction opf_ArcWeight = Subgraph::EuclideanDistance;

/*--------- Supervised OPF -------------------------------------*/
//Training function -----
void opf_OPFTraining(Subgraph *sg){
  int p,q, i;
  float tmp,weight;
  RealHeap *Q = NULL;
  std::vector<float> pathval(sg->nodes.size());

  // compute optimum prototypes
  opf_MSTPrototypes(sg);

  // initialization
  CreateRealHeap(sg->nodes.size(), pathval.data());

  for (p = 0; p < static_cast<int>(sg->nodes.size()); p++) {
    if (sg->nodes[p].status==opf_PROTOTYPE){
      sg->nodes[p].pred   = NIL;
      pathval[p]         = 0;
      sg->nodes[p].label  = sg->nodes[p].truelabel;
      InsertRealHeap(Q, p);
    }else{ // non-prototypes
      pathval[p]  = FLT_MAX;
    }
  }

  // IFT with fmax
  i=0;
  while ( !IsEmptyRealHeap(Q) ) {
    RemoveRealHeap(Q,&p);

    sg->ordered_list_of_nodes[i]=p; i++;
    sg->nodes[p].pathval = pathval[p];

    for (q=0; q < static_cast<int>(sg->nodes.size()); q++){
      if (p!=q){
	if (pathval[p] < pathval[q]){
	  if(!opf_PrecomputedDistance)
	    weight = opf_ArcWeight(sg->nodes[p].feat,sg->nodes[q].feat,sg->nfeats);
	  else
	    weight = opf_DistanceValue[sg->nodes[p].position][sg->nodes[q].position];
	  tmp  = MAX(pathval[p],weight);
	  if ( tmp < pathval[ q ] ) {
	    sg->nodes[q].pred  = p;
	    sg->nodes[q].label = sg->nodes[p].label;
	    UpdateRealHeap(Q, q, tmp);
	  }
	}
      }
    }
  }

  DestroyRealHeap( &Q );
}

//Classification function: it simply classifies samples from sg -----
void opf_OPFClassifying(Subgraph *sgtrain, Subgraph *sg)
{
  int i, j, k, l, label = -1;
  float tmp, weight, minCost;

  for (i = 0; i < static_cast<int>(sg->nodes.size()); i++)
  {
    j       = 0;
    k       = sgtrain->ordered_list_of_nodes[j];
    if(!opf_PrecomputedDistance)
      weight = opf_ArcWeight(sgtrain->nodes[k].feat,sg->nodes[i].feat,sg->nfeats);
    else
      weight = opf_DistanceValue[sgtrain->nodes[k].position][sg->nodes[i].position];

    minCost = MAX(sgtrain->nodes[k].pathval, weight);
    label   = sgtrain->nodes[k].label;

    while((j < static_cast<int>(sgtrain->nodes.size())-1)&&
    (minCost > sgtrain->nodes[sgtrain->ordered_list_of_nodes[j+1]].pathval)){

      l  = sgtrain->ordered_list_of_nodes[j+1];

      if(!opf_PrecomputedDistance)
	weight = opf_ArcWeight(sgtrain->nodes[l].feat,sg->nodes[i].feat,sg->nfeats);
      else
	weight = opf_DistanceValue[sgtrain->nodes[l].position][sg->nodes[i].position];
      tmp = MAX(sgtrain->nodes[l].pathval, weight);
      if(tmp < minCost){
	minCost = tmp;
	label = sgtrain->nodes[l].label;
      }
      j++;
      k  = l;
    }
    sg->nodes[i].label = label;
  }
}

// Semi-supervised learning function
Subgraph *opf_OPFSemiLearning(Subgraph *sg, Subgraph *nonsg, Subgraph *sgeval){

  int p,q, i, cont;
  float tmp,weight;
  RealHeap *Q = NULL;
  float *pathval = NULL;

  Subgraph *merged = opf_MergeSubgraph(sg,nonsg);
  cont = 0;

  if(sgeval != NULL) {
	
	//Learning from errors in the evaluation set
	opf_OPFLearning(&merged, &sgeval);
	
	for (i = 0; i < sg->nnodes; i++) {
		CopySNode(&sg->node[i], &merged->node[i], sg->nfeats);
	}
	
	for (i = ((merged->nnodes)-(nonsg->nnodes)); i < merged->nnodes; i++) {
		CopySNode(&nonsg->node[cont], &merged->node[i], nonsg->nfeats);
		cont++;
	}
  }
  
  // compute optimum prototypes
  opf_MSTPrototypes(sg);
  
  merged = opf_MergeSubgraph(sg,nonsg);
  
  // initialization
  pathval = AllocFloatArray(merged->nnodes);

  Q=CreateRealHeap(merged->nnodes, pathval);

  for (p = 0; p < merged->nnodes; p++) {
    if (merged->node[p].status==opf_PROTOTYPE){
      merged->node[p].pred   = NIL;
      pathval[p]         = 0;
      merged->node[p].label  = merged->node[p].truelabel;
      InsertRealHeap(Q, p);
    }else{ // non-prototypes
      pathval[p]  = FLT_MAX;
    }
  }
  // IFT with fmax
  i=0;
  while ( !IsEmptyRealHeap(Q) ) {
    RemoveRealHeap(Q,&p);

    merged->ordered_list_of_nodes[i]=p; i++;
    merged->node[p].pathval = pathval[p];

    for (q=0; q < merged->nnodes; q++){
      if (p!=q){
	if (pathval[p] < pathval[q]){
	  if(!opf_PrecomputedDistance)
	    weight = opf_ArcWeight(merged->node[p].feat,merged->node[q].feat,merged->nfeats);
	  else
	    weight = opf_DistanceValue[merged->node[p].position][merged->node[q].position];
	  tmp  = MAX(pathval[p],weight);
	  if ( tmp < pathval[ q ] ) {
	    merged->node[q].pred  = p;
	    merged->node[q].label = merged->node[p].label;
		merged->node[q].truelabel = merged->node[q].label;
	    UpdateRealHeap(Q, q, tmp);
	  }
	}
      }
    }
  }

  DestroyRealHeap( &Q );
  free( pathval );

  return merged;
}


// Classification function ----- it classifies nodes of sg by using
// the OPF-clustering labels from sgtrain

void opf_OPFKNNClassify(Subgraph *sgtrain, Subgraph *sg){
  int   i, j, k;
  float weight;

  for (i = 0; i < sg->nnodes; i++){
    for (j = 0; (j < sgtrain->nnodes); j++){
      k = sgtrain->ordered_list_of_nodes[j];
      if(!opf_PrecomputedDistance)
	weight = opf_ArcWeight(sgtrain->node[k].feat,sg->node[i].feat,sg->nfeats);
      else
	weight = opf_DistanceValue[sgtrain->node[k].position][sg->node[i].position];
      if (weight <= sgtrain->node[k].radius){
	sg->node[i].label = sgtrain->node[k].label;
	break;
      }
    }
  }

}

//Learning function: it executes the learning procedure for CompGraph replacing the
//missclassified samples in the evaluation set by non prototypes from
//training set -----
void opf_OPFLearning(Subgraph **sgtrain, Subgraph **sgeval){
	int i = 0, iterations = 10;
	float Acc = FLT_MIN, AccAnt = FLT_MIN,MaxAcc=FLT_MIN, delta;
	Subgraph *sg=NULL;

	do{
		AccAnt = Acc;
		fflush(stdout); fprintf(stdout, "\nrunning iteration ... %d ", i);
		opf_OPFTraining(*sgtrain);
		opf_OPFClassifying(*sgtrain, *sgeval);
		Acc = opf_Accuracy(*sgeval);
		if (Acc > MaxAcc){
		  MaxAcc = Acc;
		  if (sg!=NULL) DestroySubgraph(&sg);
		  sg = CopySubgraph(*sgtrain);
		}
		opf_SwapErrorsbyNonPrototypes(&(*sgtrain), &(*sgeval));
		fflush(stdout); fprintf(stdout,"opf_Accuracy in the evaluation set: %.2f %%\n", Acc*100);
		i++;
		delta = fabs(Acc-AccAnt);
	}while ((delta > 0.0001) && (i <= iterations));
	DestroySubgraph(&(*sgtrain));
	*sgtrain = sg;
}


void opf_OPFAgglomerativeLearning(Subgraph **sgtrain, Subgraph **sgeval){
    int n, i = 1;
    float Acc;

    /*while  there exists misclassified samples in sgeval*/
    do{
        fflush(stdout); fprintf(stdout, "\nrunning iteration ... %d ", i++);
        n = 0;
        opf_OPFTraining(*sgtrain);
        opf_OPFClassifying(*sgtrain, *sgeval);
        Acc = opf_Accuracy(*sgeval); fprintf(stdout," %f",Acc*100);
        opf_MoveMisclassifiedNodes(&(*sgeval), &(*sgtrain), &n);
        fprintf(stdout,"\nMisclassified nodes: %d",n);
    }while(n);
}

/*--------- UnSupervised OPF -------------------------------------*/
//Training function: it computes unsupervised training for the
//pre-computed best k.

void opf_OPFClustering(Subgraph *sg){
    int p, q, l;
    float tmp;
    std::vector<float> pathval(sg->nodes.size());
    RealHeap *Q = CreateRealHeap(sg->nodes.size(), pathval.data());
    SetRemovalPolicyRealHeap(Q, MAXVALUE);

    // Add arcs to guarantee symmetry on plateaus
    for (size_t i = 0; i < sg->nodes.size(); i++) {
        for (int j : sg->nodes[i].adj) {
            if (sg->nodes[i].dens == sg->nodes[j].dens) {
                // insert i in the adjacency of j if it is not there.
                bool found = false;
                for (int neighbor : sg->nodes[j].adj) {
                    if (neighbor == static_cast<int>(i)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    sg->nodes[j].adj.push_back(i);
                }
            }
        }
    }

    // Compute clustering
    for (size_t i = 0; i < sg->nodes.size(); ++i) {
        pathval[i] = sg->nodes[i].pathval;
        sg->nodes[i].pred = NIL;
        sg->nodes[i].root = i;
        InsertRealHeap(Q, i);
    }

    l = 0;
    int i = 0;
    while (!IsEmptyRealHeap(Q)) {
        RemoveRealHeap(Q, &p);
        sg->ordered_list_of_nodes[i] = p;
        i++;

        if (sg->nodes[p].pred == NIL) {
            pathval[p] = sg->nodes[p].dens;
            sg->nodes[p].label = l;
            l++;
        }

        sg->nodes[p].pathval = pathval[p];
        for (int q_neighbor : sg->nodes[p].adj) {
            if (Q->color[q_neighbor] != BLACK) {
                tmp = MIN(pathval[p], sg->nodes[q_neighbor].dens);
                if (tmp > pathval[q_neighbor]) {
                    UpdateRealHeap(Q, q_neighbor, tmp);
                    sg->nodes[q_neighbor].pred = p;
                    sg->nodes[q_neighbor].root = sg->nodes[p].root;
                    sg->nodes[q_neighbor].label = sg->nodes[p].label;
                }
            }
        }
    }

    sg->nlabels = l;

    DestroyRealHeap(&Q);
}

/*------------ Auxiliary functions ------------------------------ */
//Resets subgraph fields (pred and arcs)
void opf_ResetSubgraph(Subgraph *sg){
  int i;

  for (i = 0; i < sg->nnodes; i++)
    sg->node[i].pred    = NIL;
  opf_DestroyArcs(sg);
}

//Replace errors from evaluating set by non prototypes from training set
void opf_SwapErrorsbyNonPrototypes(Subgraph **sgtrain, Subgraph **sgeval){
  int i, j, counter, nonprototypes = 0, nerrors = 0;

  for (i = 0; i < (*sgtrain)->nnodes; i++){
    if((*sgtrain)->node[i].pred != NIL){ // non prototype
      nonprototypes++;
    }
  }

  for (i = 0; i < (*sgeval)->nnodes; i++)
    if((*sgeval)->node[i].label != (*sgeval)->node[i].truelabel) nerrors++;

  for (i = 0; i < (*sgeval)->nnodes && nonprototypes >0 && nerrors > 0; i++){
    if((*sgeval)->node[i].label != (*sgeval)->node[i].truelabel){
      counter = nonprototypes;
      while(counter > 0){
	j = RandomInteger(0,(*sgtrain)->nnodes-1);
	if ((*sgtrain)->node[j].pred!=NIL)
	  {
	    SwapSNode(&((*sgtrain)->node[j]), &((*sgeval)->node[i]));
	    (*sgtrain)->node[j].pred = NIL;
	    nonprototypes--;
	    nerrors--;
	    counter = 0;
	  }
	else counter--;
      }
    }
  }
}

//mark nodes and the whole path as relevants
void opf_MarkNodes(Subgraph *g, int i){
  while(g->node[i].pred != NIL){
    g->node[i].relevant = 1;
    i = g->node[i].pred;
  }
  g->node[i].relevant = 1;
}

// Remove irrelevant nodes
void opf_RemoveIrrelevantNodes(Subgraph **sg){
  Subgraph *newsg = NULL;
  int i,k,num_of_irrelevants=0;

  for (i=0; i < (*sg)->nnodes; i++) {
    if (!(*sg)->node[i].relevant)
      num_of_irrelevants++;
  }

  if (num_of_irrelevants>0){
    newsg = CreateSubgraph((*sg)->nnodes - num_of_irrelevants);
    newsg->nfeats = (*sg)->nfeats;
//    for (i=0; i < newsg->nnodes; i++)
//      newsg->node[i].feat = AllocFloatArray(newsg->nfeats);

    k=0;
    newsg->nlabels = (*sg)->nlabels;
    for (i=0; i < (*sg)->nnodes; i++){
      if ((*sg)->node[i].relevant){// relevant node
	CopySNode(&(newsg->node[k]), &((*sg)->node[i]), newsg->nfeats);
	k++;
      }
    }
    newsg->nlabels=(*sg)->nlabels;
    DestroySubgraph(sg);
    *sg=newsg;
  }
}

//Move irrelevant nodes from source graph (src) to destiny graph (dst)
void opf_MoveIrrelevantNodes(Subgraph **src, Subgraph **dst){
  int i, j, k, num_of_irrelevants=0;
  Subgraph *newsrc = NULL, *newdst = NULL;

  for (i=0; i < (*src)->nnodes; i++){
    if (!(*src)->node[i].relevant)
      num_of_irrelevants++;
  }

  if (num_of_irrelevants>0){
    newsrc = CreateSubgraph((*src)->nnodes-num_of_irrelevants);
    newdst = CreateSubgraph((*dst)->nnodes+num_of_irrelevants);

    newsrc->nfeats = (*src)->nfeats; newdst->nfeats = (*dst)->nfeats;
    newsrc->nlabels = (*src)->nlabels; newdst->nlabels = (*dst)->nlabels;

//    for (i=0; i < newsrc->nnodes; i++)
//      newsrc->node[i].feat = AllocFloatArray(newsrc->nfeats);

//    for (i=0; i < newdst->nnodes; i++)
//      newdst->node[i].feat = AllocFloatArray(newdst->nfeats);

    for (i = 0; i < (*dst)->nnodes; i++)
      CopySNode(&(newdst->node[i]), &((*dst)->node[i]), newdst->nfeats);
    j=i;

    k = 0;
    for (i=0; i < (*src)->nnodes; i++){
      if ((*src)->node[i].relevant)// relevant node
	CopySNode(&(newsrc->node[k++]), &((*src)->node[i]), newsrc->nfeats);
      else
	CopySNode(&(newdst->node[j++]), &((*src)->node[i]), newdst->nfeats);
		}
    DestroySubgraph(&(*src));
    DestroySubgraph(&(*dst));
    *src = newsrc;
    *dst = newdst;
  }
}

//Move misclassified nodes from source graph (src) to destiny graph (dst)
void opf_MoveMisclassifiedNodes(Subgraph **src, Subgraph **dst,  int *p){
  int i, j, k, num_of_misclassified=0;
  Subgraph *newsrc = NULL, *newdst = NULL;

  for (i=0; i < (*src)->nnodes; i++){
    if ((*src)->node[i].truelabel != (*src)->node[i].label)
      num_of_misclassified++;
  }
  *p = num_of_misclassified;

  if (num_of_misclassified>0){
    newsrc = CreateSubgraph((*src)->nnodes-num_of_misclassified);
    newdst = CreateSubgraph((*dst)->nnodes+num_of_misclassified);

    newsrc->nfeats = (*src)->nfeats; newdst->nfeats = (*dst)->nfeats;
    newsrc->nlabels = (*src)->nlabels; newdst->nlabels = (*dst)->nlabels;

    for (i = 0; i < (*dst)->nnodes; i++)
      CopySNode(&(newdst->node[i]), &((*dst)->node[i]), newdst->nfeats);
    j=i;

    k = 0;
    for (i=0; i < (*src)->nnodes; i++){
      if ((*src)->node[i].truelabel == (*src)->node[i].label)// misclassified node
	CopySNode(&(newsrc->node[k++]), &((*src)->node[i]), newsrc->nfeats);
      else
	CopySNode(&(newdst->node[j++]), &((*src)->node[i]), newdst->nfeats);
		}
    DestroySubgraph(&(*src));
    DestroySubgraph(&(*dst));
    *src = newsrc;
    *dst = newdst;
  }
}

//write model file to disk
void opf_WriteModelFile(Subgraph *g, char *file){
  FILE *fp = NULL;
  int   i, j;

  fp = fopen(file, "wb");
  fwrite(&g->nnodes, sizeof(int), 1, fp);
   fwrite(&g->nlabels, sizeof(int), 1, fp);
   fwrite(&g->nfeats, sizeof(int), 1, fp);

  /*writing df*/
   fwrite(&g->df, sizeof(float), 1, fp);

  // for supervised opf based on pdf

   fwrite(&g->K, sizeof(float), 1, fp);
   fwrite(&g->mindens, sizeof(float), 1, fp);
   fwrite(&g->maxdens, sizeof(float), 1, fp);

  /*writing position(id), label, pred, pathval and features*/
  for (i = 0; i < g->nnodes; i++){
     fwrite(&g->node[i].position, sizeof(int), 1, fp);
     fwrite(&g->node[i].truelabel, sizeof(int), 1, fp);
     fwrite(&g->node[i].pred, sizeof(int), 1, fp);
     fwrite(&g->node[i].label, sizeof(int), 1, fp);
     fwrite(&g->node[i].pathval, sizeof(float), 1, fp);
     fwrite(&g->node[i].radius, sizeof(float), 1, fp);

    for (j = 0; j < g->nfeats; j++){
       fwrite(&g->node[i].feat[j], sizeof(float), 1, fp);
    }
  }

  for (i = 0; i < g->nnodes; i++){
     fwrite(&g->ordered_list_of_nodes[i], sizeof(int), 1, fp);
  }

  fclose(fp);
}

//read subgraph from opf model file
Subgraph *opf_ReadModelFile(char *file){
  Subgraph *g = NULL;
  FILE *fp = NULL;
  int nnodes, i, j;
  char msg[256];

  if((fp = fopen(file, "rb")) == NULL){
    sprintf(msg, "%s%s", "Unable to open file ", file);
    Error(msg,"ReadSubGraph");
  }

  /*reading # of nodes, classes and feats*/
  if (fread(&nnodes, sizeof(int), 1, fp) != 1) 
    Error("Could not read number of nodes","opf_ReadModelFile");

  g = CreateSubgraph(nnodes);
  if (fread(&g->nlabels, sizeof(int), 1, fp) != 1) 
    Error("Could not read number of labels","opf_ReadModelFile");

  if(fread(&g->nfeats, sizeof(int), 1, fp) != 1) 
    Error("Could not read number of features","opf_ReadModelFile");


  /*reading df*/
  if (fread(&g->df, sizeof(float), 1, fp) != 1) 
    Error("Could not read adjacency radius","opf_ReadModelFile");


  // for supervised opf by pdf

  if (fread(&g->K, sizeof(float), 1, fp) != 1) 
    Error("Could not read the best K","opf_ReadModelFile");
    
  if (fread(&g->mindens, sizeof(float), 1, fp) != 1) 
    Error("Could not read minimum density","opf_ReadModelFile");

  if (fread(&g->maxdens, sizeof(float), 1, fp) != 1) 
    Error("Could not read maximum density","opf_ReadModelFile");

  /*reading features*/
  for (i = 0; i < g->nnodes; i++){
    g->node[i].feat = (float *)malloc(g->nfeats*sizeof(float));
    if (fread(&g->node[i].position, sizeof(int), 1, fp) != 1) 
      Error("Could not read node position","opf_ReadModelFile");
    if (fread(&g->node[i].truelabel, sizeof(int), 1, fp) != 1) 
      Error("Could not read node true label","opf_ReadModelFile");
    if (fread(&g->node[i].pred, sizeof(int), 1, fp) != 1)
      Error("Could not read node predecessor","opf_ReadModelFile");
    if (fread(&g->node[i].label, sizeof(int), 1, fp) != 1) 
      Error("Could not read node label","opf_ReadModelFile");
    if (fread(&g->node[i].pathval, sizeof(float), 1, fp) != 1) 
      Error("Could not read node path value","opf_ReadModelFile");
    if (fread(&g->node[i].radius, sizeof(float), 1, fp) != 1) 
      Error("Could not read node adjacency radius","opf_ReadModelFile");

    for (j = 0; j < g->nfeats; j++){
      if (fread(&g->node[i].feat[j], sizeof(float), 1, fp) != 1) 
	Error("Could not read node features","opf_ReadModelFile");
    }
  }

  for (i = 0; i < g->nnodes; i++){
    if (fread(&g->ordered_list_of_nodes[i], sizeof(int), 1, fp) != 1) 
      Error("Could not read ordered list of nodes","opf_ReadModelFile");
  }

  fclose(fp);

  return g;
}

//normalize features
void opf_NormalizeFeatures(Subgraph *sg){
  float *mean = (float *)calloc(sg->nfeats,sizeof(float)), *std = (float *)calloc(sg->nfeats, sizeof(int));
  int i,j;

  for (i = 0; i < sg->nfeats; i++){
    for (j = 0; j < sg->nnodes; j++)
      mean[i]+=sg->node[j].feat[i]/sg->nnodes;
    for (j = 0; j < sg->nnodes; j++)
      std[i]+=pow(sg->node[j].feat[i]-mean[i],2)/sg->nnodes;
    std[i]=sqrt(std[i]);
	if(std[i] == 0) 	std[i] = 1.0;
  }

  for (i = 0; i < sg->nfeats; i++){
    for (j = 0; j < sg->nnodes; j++)
      sg->node[j].feat[i] = (sg->node[j].feat[i]-mean[i])/std[i];
  }

  free(mean);
  free(std);
}

// Find prototypes by the MST approach
void opf_MSTPrototypes(Subgraph *sg){
  int p,q;
  float weight;
  RealHeap *Q=NULL;
  float *pathval = NULL;
  int  pred;
  float nproto;

  // initialization
  pathval = AllocFloatArray(sg->nnodes);
  Q = CreateRealHeap(sg->nnodes, pathval);

  for (p = 0; p < sg->nnodes; p++) {
    pathval[ p ] = FLT_MAX;
    sg->node[p].status=0;
  }

  pathval[0]  = 0;
  sg->node[0].pred = NIL;
  InsertRealHeap(Q, 0);

  nproto=0.0;

  // Prim's algorithm for Minimum Spanning Tree
  while ( !IsEmptyRealHeap(Q) ) {
    RemoveRealHeap(Q,&p);
    sg->node[p].pathval = pathval[p];

    pred=sg->node[p].pred;
    if (pred!=NIL)
      if (sg->node[p].truelabel != sg->node[pred].truelabel){
	if (sg->node[p].status!=opf_PROTOTYPE){
	  sg->node[p].status=opf_PROTOTYPE;
	  nproto++;
	}
	if (sg->node[pred].status!=opf_PROTOTYPE){
	  sg->node[pred].status=opf_PROTOTYPE;
	  nproto++;
	}
      }

    for (q=0; q < sg->nnodes; q++){
      if (Q->color[q]!=BLACK){
	if (p!=q){
	  if(!opf_PrecomputedDistance)
	    weight = opf_ArcWeight(sg->node[p].feat,sg->node[q].feat,sg->nfeats);
	  else
	    weight = opf_DistanceValue[sg->node[p].position][sg->node[q].position];
	  if ( weight < pathval[ q ] ) {
	    sg->node[q].pred = p;
	    UpdateRealHeap(Q, q, weight);
	  }
	}
      }
    }
  }
  DestroyRealHeap(&Q);
  free( pathval );

}

//It creates k folds for cross validation
Subgraph **kFoldSubgraph(Subgraph *sg, int k){
	Subgraph **out = (Subgraph **)malloc(k*sizeof(Subgraph *));
	int totelems, foldsize = 0, i, *label = (int *)calloc((sg->nlabels+1),sizeof(int));
	int *nelems = (int *)calloc((sg->nlabels+1),sizeof(int)), j, z, w, m, n;
	int *nelems_aux = (int *)calloc((sg->nlabels+1),sizeof(int)), *resto = (int *)calloc((sg->nlabels+1),sizeof(int));

	for (i=0; i < sg->nnodes; i++){
	    sg->node[i].status = 0;
	    label[sg->node[i].truelabel]++;
	}

	for (i=0; i < sg->nnodes; i++)
	    nelems[sg->node[i].truelabel]=MAX((int)((1/(float)k)*label[sg->node[i].truelabel]),1);

	for (i = 1; i <= sg->nlabels; i++){
		foldsize+=nelems[i];
	    	nelems_aux[i]=nelems[i];
		resto[i] = label[i] - k*nelems_aux[i];
	}

	for (i = 0; i < k-1; i++){
		out[i] = CreateSubgraph(foldsize);
		out[i]->nfeats = sg->nfeats;
		out[i]->nlabels = sg->nlabels;
		for (j = 0; j < foldsize; j++)
			out[i]->node[j].feat = (float *)malloc(sg->nfeats*sizeof(float));
	}

	totelems = 0;
	for (j = 1; j <= sg->nlabels; j++)
		totelems+=resto[j];

	out[i] = CreateSubgraph(foldsize+totelems);
	out[i]->nfeats = sg->nfeats;
	out[i]->nlabels = sg->nlabels;

	for (j = 0; j < foldsize+totelems; j++)
			out[i]->node[j].feat = (float *)malloc(sg->nfeats*sizeof(float));

	for (i = 0; i < k; i++){
		totelems=0;
		if (i == k-1){
			for (w = 1; w <= sg->nlabels; w++){
	    			nelems_aux[w]+=resto[w];
				totelems += nelems_aux[w];
			}
		}
		else{
			for (w = 1; w <= sg->nlabels; w++)
				totelems += nelems_aux[w];
		}

		for (w = 1; w <= sg->nlabels; w++)
	    		nelems[w]=nelems_aux[w];


		z = 0;	m = 0;
		while(totelems > 0){
			if(i == k-1){
				for (w = m; w < sg->nnodes; w++){
					if (sg->node[w].status != NIL){
						j = w;
						m = w+1;
						break;
					}
				}

			}else j = RandomInteger(0,sg->nnodes-1);
			if (sg->node[j].status!=NIL){
				if (nelems[sg->node[j].truelabel]>0){
					out[i]->node[z].position = sg->node[j].position;
					for (n=0; n < sg->nfeats; n++)
						out[i]->node[z].feat[n]=sg->node[j].feat[n];
					out[i]->node[z].truelabel = sg->node[j].truelabel;
					nelems[sg->node[j].truelabel] = nelems[sg->node[j].truelabel] - 1;
					sg->node[j].status = NIL;
					totelems--;
					z++;
				}
			}
		}
	}

	free(label);
	free(nelems);
	free(nelems_aux);
	free(resto);

	return out;
}

//It creates k folds for cross validation
Subgraph **opf_kFoldSubgraph(Subgraph *sg, int k){
	Subgraph **out = (Subgraph **)malloc(k*sizeof(Subgraph *));
	int totelems, foldsize = 0, i, *label = (int *)calloc((sg->nlabels+1),sizeof(int));
	int *nelems = (int *)calloc((sg->nlabels+1),sizeof(int)), j, z, w, m, n;
	int *nelems_aux = (int *)calloc((sg->nlabels+1),sizeof(int)), *resto = (int *)calloc((sg->nlabels+1),sizeof(int));

	for (i=0; i < sg->nnodes; i++){
	    sg->node[i].status = 0;
	    label[sg->node[i].truelabel]++;
	}

	for (i=0; i < sg->nnodes; i++)
	    nelems[sg->node[i].truelabel]=MAX((int)((1/(float)k)*label[sg->node[i].truelabel]),1);

	for (i = 1; i <= sg->nlabels; i++){
		foldsize+=nelems[i];
	    	nelems_aux[i]=nelems[i];
		resto[i] = label[i] - k*nelems_aux[i];
	}

	for (i = 0; i < k-1; i++){
		out[i] = CreateSubgraph(foldsize);
		out[i]->nfeats = sg->nfeats;
		out[i]->nlabels = sg->nlabels;
		for (j = 0; j < foldsize; j++)
			out[i]->node[j].feat = (float *)malloc(sg->nfeats*sizeof(float));
	}

	totelems = 0;
	for (j = 1; j <= sg->nlabels; j++)
		totelems+=resto[j];

	out[i] = CreateSubgraph(foldsize+totelems);
	out[i]->nfeats = sg->nfeats;
	out[i]->nlabels = sg->nlabels;

	for (j = 0; j < foldsize+totelems; j++)
			out[i]->node[j].feat = (float *)malloc(sg->nfeats*sizeof(float));

	for (i = 0; i < k; i++){
		totelems=0;
		if (i == k-1){
			for (w = 1; w <= sg->nlabels; w++){
	    			nelems_aux[w]+=resto[w];
				totelems += nelems_aux[w];
			}
		}
		else{
			for (w = 1; w <= sg->nlabels; w++)
				totelems += nelems_aux[w];
		}

		for (w = 1; w <= sg->nlabels; w++)
	    		nelems[w]=nelems_aux[w];


		z = 0;	m = 0;
		while(totelems > 0){
			if(i == k-1){
				for (w = m; w < sg->nnodes; w++){
					if (sg->node[w].status != NIL){
						j = w;
						m = w+1;
						break;
					}
				}

			}else j = RandomInteger(0,sg->nnodes-1);
			if (sg->node[j].status!=NIL){
				if (nelems[sg->node[j].truelabel]>0){
					out[i]->node[z].position = sg->node[j].position;
					for (n=0; n < sg->nfeats; n++)
						out[i]->node[z].feat[n]=sg->node[j].feat[n];
					out[i]->node[z].truelabel = sg->node[j].truelabel;
					nelems[sg->node[j].truelabel] = nelems[sg->node[j].truelabel] - 1;
					sg->node[j].status = NIL;
					totelems--;
					z++;
				}
			}
		}
	}

	free(label);
	free(nelems);
	free(nelems_aux);
	free(resto);

	return out;
}


// Split subgraph into two parts such that the size of the first part
// is given by a percentual of samples.
void opf_SplitSubgraph(Subgraph *sg, Subgraph **sg1, Subgraph **sg2, float perc1){
  int *label=AllocIntArray(sg->nlabels+1),i,j,i1,i2;
  int *nelems=AllocIntArray(sg->nlabels+1),totelems;
  srandom((int)time(NULL));

  for (i=0; i < sg->nnodes; i++) {
    sg->node[i].status = 0;
    label[sg->node[i].truelabel]++;
  }

  for (i=0; i < sg->nnodes; i++) {
    nelems[sg->node[i].truelabel]=MAX((int)(perc1*label[sg->node[i].truelabel]),1);
  }

  free(label);

  totelems=0;
  for (j=1; j <= sg->nlabels; j++)
    totelems += nelems[j];

  *sg1 = CreateSubgraph(totelems);
  *sg2 = CreateSubgraph(sg->nnodes-totelems);
  (*sg1)->nfeats = sg->nfeats;
  (*sg2)->nfeats = sg->nfeats;

  for (i1=0; i1 < (*sg1)->nnodes; i1++)
    (*sg1)->node[i1].feat = AllocFloatArray((*sg1)->nfeats);
  for (i2=0; i2 < (*sg2)->nnodes; i2++)
    (*sg2)->node[i2].feat = AllocFloatArray((*sg2)->nfeats);

  (*sg1)->nlabels = sg->nlabels;
  (*sg2)->nlabels = sg->nlabels;

  i1=0;
  while(totelems > 0){
    i = RandomInteger(0,sg->nnodes-1);
    if (sg->node[i].status!=NIL){
      if (nelems[sg->node[i].truelabel]>0){// copy node to sg1
	(*sg1)->node[i1].position = sg->node[i].position;
	for (j=0; j < (*sg1)->nfeats; j++)
	  (*sg1)->node[i1].feat[j]=sg->node[i].feat[j];
	(*sg1)->node[i1].truelabel = sg->node[i].truelabel;
	i1++;
	nelems[sg->node[i].truelabel] = nelems[sg->node[i].truelabel] - 1;
	sg->node[i].status = NIL;
	totelems--;
      }
    }
  }

  i2=0;
  for (i=0; i < sg->nnodes; i++){
    if (sg->node[i].status!=NIL){
      (*sg2)->node[i2].position = sg->node[i].position;
      for (j=0; j < (*sg2)->nfeats; j++)
	(*sg2)->node[i2].feat[j]=sg->node[i].feat[j];
      (*sg2)->node[i2].truelabel = sg->node[i].truelabel;
      i2++;
    }
  }

  free(nelems);
}

//Merge two subgraphs
Subgraph *opf_MergeSubgraph(Subgraph *sg1, Subgraph *sg2){
	if(sg1->nfeats != sg2->nfeats) Error("Invalid number of feats!","MergeSubgraph");

	Subgraph *out = CreateSubgraph(sg1->nnodes+sg2->nnodes);
	int i = 0, j;

	if(sg1->nlabels > sg2->nlabels)	out->nlabels = sg1->nlabels;
	else out->nlabels = sg2->nlabels;
	out->nfeats = sg1->nfeats;

	for (i = 0; i < sg1->nnodes; i++)
		CopySNode(&out->node[i], &sg1->node[i], out->nfeats);
	for (j = 0; j < sg2->nnodes; j++){
		CopySNode(&out->node[i], &sg2->node[j], out->nfeats);
		i++;
	}

	return out;
}

// Compute accuracy
float opf_Accuracy(Subgraph *sg){
	float Acc = 0.0f, **error_matrix = NULL, error = 0.0f;
	int i, *nclass = NULL, nlabels=0;

	error_matrix = (float **)calloc(sg->nlabels+1, sizeof(float *));
	for(i=0; i<= sg->nlabels; i++)
	  error_matrix[i] = (float *)calloc(2, sizeof(float));

	nclass = AllocIntArray(sg->nlabels+1);

	for (i = 0; i < sg->nnodes; i++){
	  nclass[sg->node[i].truelabel]++;
	}

	for (i = 0; i < sg->nnodes; i++){
	  if(sg->node[i].truelabel != sg->node[i].label){
	    error_matrix[sg->node[i].truelabel][1]++;
	    error_matrix[sg->node[i].label][0]++;
	  }
	}

	for(i=1; i <= sg->nlabels; i++){
	  if (nclass[i]!=0){
	    error_matrix[i][1] /= (float)nclass[i];
	    error_matrix[i][0] /= (float)(sg->nnodes - nclass[i]);
	    nlabels++;
	  }
	}

	for(i=1; i <= sg->nlabels; i++){
	  if (nclass[i]!=0)
	    error += (error_matrix[i][0]+error_matrix[i][1]);
	}

	Acc = 1.0-(error/(2.0*nlabels));

	for(i=0; i <= sg->nlabels; i++)
	  free(error_matrix[i]);
	free(error_matrix);
	free(nclass);

	return(Acc);
}

// Compute the confusion matrix
int **opf_ConfusionMatrix(Subgraph *sg){
	int **opf_ConfusionMatrix = NULL, i;

	opf_ConfusionMatrix = (int **)calloc((sg->nlabels+1),sizeof(int *));
	for (i = 1; i <= sg->nlabels; i++)
		opf_ConfusionMatrix[i] = (int *)calloc((sg->nlabels+1),sizeof(int));

	for (i = 0; i < sg->nnodes; i++)
		opf_ConfusionMatrix[sg->node[i].truelabel][sg->node[i].label]++;

	return opf_ConfusionMatrix;
}


//read distances from precomputed distances file
float **opf_ReadDistances(char *fileName, int *n)
{
  int nsamples, i;
  FILE *fp = NULL;
  float **M = NULL;
  char msg[256];
  
  fp = fopen(fileName,"rb");
  
  if(fp == NULL){
    sprintf(msg, "%s%s", "Unable to open file ", fileName);
    Error(msg,"opf_ReadDistances");
  }
  
  if (fread(&nsamples, sizeof(int), 1, fp) != 1) 
    Error("Could not read number of samples","opf_ReadDistances");
  
  *n = nsamples;
  M = (float **)malloc(nsamples*sizeof(float *));
  
  for (i = 0; i < nsamples; i++)
    {
      M[i] = (float *) malloc( nsamples*sizeof(float));
      if( fread(M[i], sizeof(float), nsamples, fp) != nsamples ){ 
	Error("Could not read samples","opf_ReadDistances");		
      }
    }
  fclose(fp);

  return M;
}

// Normalized cut
float opf_NormalizedCut( Subgraph *sg ){
    int l, p, q;
    float ncut, dist;
    std::vector<float> acumIC(sg->nlabels, 0.0f); //acumulate weights inside each class
    std::vector<float> acumEC(sg->nlabels, 0.0f); //acumulate weights between the class and a distinct one

    ncut = 0.0;

    for ( p = 0; p < static_cast<int>(sg->nodes.size()); p++ )
    {
        for ( int q_neighbor : sg->nodes[p].adj )
        {
            q = q_neighbor;
	    if (!opf_PrecomputedDistance)
	      dist = opf_ArcWeight(sg->nodes[p].feat,sg->nodes[q].feat,sg->nfeats);
	    else
	      dist = opf_DistanceValue[sg->nodes[p].position][sg->nodes[q].position];
            if ( dist > 0.0 )
            {
                if ( sg->nodes[p].label == sg->nodes[q].label )
                {
                    acumIC[ sg->nodes[p].label ] += 1.0 / dist; // intra-class weight
                }
                else   // inter - class weight
                {
                    acumEC[ sg->nodes[p].label ] += 1.0 / dist; // inter-class weight
                }
            }
        }
    }

    for ( l = 0; l < static_cast<int>(sg->nlabels); l++ )
    {
        if ( acumIC[ l ] + acumEC[ l ]  > 0.0 ) ncut += (float) acumEC[ l ] / ( acumIC[ l ] + acumEC[ l ] );
    }
    return( ncut );
}

// Estimate the best k by minimum cut
void opf_BestkMinCut(Subgraph *sg, int kmin, int kmax)
{
    int k, bestk = kmax;
    float mincut=FLT_MAX,nc;

    float* maxdists = opf_CreateArcs2(sg,kmax); // stores the maximum distances for every k=1,2,...,kmax

    // Find the best k
    for (k = kmin; (k <= kmax)&&(mincut != 0.0); k++)
    {
        sg->df = maxdists[k-1];
        sg->bestk = k;

		opf_PDFtoKmax(sg);

        opf_OPFClusteringToKmax(sg);

        nc = opf_NormalizedCutToKmax(sg);

        if (nc < mincut)
        {
            mincut=nc;
            bestk = k;
        }
    }
    free(maxdists);
    opf_DestroyArcs(sg);

    sg->bestk = bestk;

    opf_CreateArcs(sg,sg->bestk);
    opf_PDF(sg);

    printf("best k %d ",sg->bestk);
}


// Create adjacent list in subgraph: a knn graph
void opf_CreateArcs(Subgraph *sg, int knn){
    int    j,l,k;
    float  dist;
    std::vector<int> nn(knn + 1);
    std::vector<float> d(knn + 1);

    /* Create graph with the knn-nearest neighbors */

    sg->df=0.0;
    for (size_t i=0; i < sg->nodes.size(); i++)
    {
        for (l=0; l < knn; l++)
            d[l]=FLT_MAX;
        for (j=0; j < static_cast<int>(sg->nodes.size()); j++)
        {
            if (j!=static_cast<int>(i))
            {
	      if (!opf_PrecomputedDistance)
		d[knn] = opf_ArcWeight(sg->nodes[i].feat,sg->nodes[j].feat,sg->nfeats);
	      else
		d[knn] = opf_DistanceValue[sg->nodes[i].position][sg->nodes[j].position];
	      nn[knn]= j;
	      k      = knn;
	      while ((k > 0)&&(d[k]<d[k-1]))
                {
		  dist    = d[k];
		  l       = nn[k];
		  d[k]    = d[k-1];
		  nn[k]   = nn[k-1];
		  d[k-1]  = dist;
		  nn[k-1] = l;
		  k--;
                }
            }
        }

        for (l=0; l < knn; l++)
        {
            if (d[l]!=INT_MAX)
            {
                if (d[l] > sg->df)
                    sg->df = d[l];
		sg->nodes[i].radius = d[l];
                sg->nodes[i].adj.push_back(nn[l]);
            }
        }
    }

    if (sg->df<0.00001)
        sg->df = 1.0;
}

// Destroy Arcs
void opf_DestroyArcs(Subgraph *sg){
    for (auto& node : sg->nodes)
    {
        node.nplatadj = 0;
        node.adj.clear();
    }
}

// opf_PDF computation
void opf_PDF(Subgraph *sg){
    double  dist;
    std::vector<float> value(sg->nodes.size());

    sg->K    = (2.0*(float)sg->df/9.0);
    sg->mindens = FLT_MAX;
    sg->maxdens = FLT_MIN;
    for (size_t i=0; i < sg->nodes.size(); i++)
    {
        value[i]=0.0;
        int nelems=1;
        for(int neighbor : sg->nodes[i].adj)
        {
            if (!opf_PrecomputedDistance)
	      		dist  = opf_ArcWeight(sg->nodes[i].feat,sg->nodes[neighbor].feat,sg->nfeats);
            else
	      		dist  = opf_DistanceValue[sg->nodes[i].position][sg->nodes[neighbor].position];
            value[i] += exp(-dist/sg->K);
            nelems++;
        }

        value[i] = (value[i]/(float)nelems);

        if (value[i] < sg->mindens)
            sg->mindens = value[i];
        if (value[i] > sg->maxdens)
            sg->maxdens = value[i];
    }

    if (sg->mindens==sg->maxdens)
    {
        for (size_t i=0; i < sg->nodes.size(); i++)
        {
	  sg->nodes[i].dens = opf_MAXDENS;
	  sg->nodes[i].pathval= opf_MAXDENS-1;
        }
    }
    else
      {
        for (size_t i=0; i < sg->nodes.size(); i++)
	  {
	    sg->nodes[i].dens = ((float)(opf_MAXDENS-1)*(value[i]-sg->mindens)/(float)(sg->maxdens-sg->mindens))+1.0;
            sg->nodes[i].pathval=sg->nodes[i].dens-1;
        }
    }
}

// Eliminate maxima in the graph with pdf below H
void opf_ElimMaxBelowH(Subgraph *sg, float H){
    int i;

    if (H>0.0)
    {
        for (i=0; i < sg->nnodes; i++)
            sg->node[i].pathval = MAX(sg->node[i].dens - H,0);
    }
}

//Eliminate maxima in the graph with area below A
void opf_ElimMaxBelowArea(Subgraph *sg, int A){
    int i, *area;

    area = SgAreaOpen(sg,A);
    for (i=0; i < sg->nnodes; i++)
    {
        sg->node[i].pathval    = MAX(area[i] - 1,0);
    }

    free(area);
}

// Eliminate maxima in the graph with volume below V
void opf_ElimMaxBelowVolume(Subgraph *sg, int V){
    int i, *volume=NULL;

    volume = SgVolumeOpen(sg,V);
    for (i=0; i < sg->nnodes; i++)
    {
        sg->node[i].pathval  = MAX(volume[i] - 1,0);
    }

    free(volume);
}

/*------------ Distance functions ------------------------------ */

// Compute Euclidean distance between feature vectors
float opf_EuclDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f;

  for (i=0; i < n; i++)
    dist += (f1[i]-f2[i])*(f1[i]-f2[i]);

  return(dist);
}


// Discretizes original distance
float opf_EuclDistLog(float *f1, float *f2, int n){
  return(((float)opf_MAXARCW*log(opf_EuclDist(f1,f2,n)+1)));
}

// Compute gaussian distance between feature vectors
 float opf_GaussDist(float *f1, float *f2, int n, float gamma){
  int i;
  float dist=0.0f;

  for (i=0; i < n; i++)
    dist += (f1[i]-f2[i])*(f1[i]-f2[i]);

  dist = exp(-gamma*sqrtf(dist));

  return(dist);
}

// Compute  chi-squared distance between feature vectors
float opf_ChiSquaredDist(float *f1, float *f2, int n){
	int i;
	float dist=0.0f, sf1 = 0.0f, sf2 = 0.0f;

	for (i = 0; i < n; i++){
		sf1+=f1[i];
		sf2+=f2[i];
	}

	for (i=0; i < n; i++)
		dist += 1/(f1[i]+f2[i]+0.000000001)*pow(f1[i]/sf1-f2[i]/sf2,2);

  return(sqrtf(dist));
}

// Compute  Manhattan distance between feature vectors
float opf_ManhattanDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f;

  for (i=0; i < n; i++)
    dist += fabs(f1[i]-f2[i]);

  return(dist);
}

// Compute  Camberra distance between feature vectors
float opf_CanberraDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f, aux;

  for (i=0; i < n; i++){
	  aux = fabs(f1[i]+f2[i]);
	  if(aux > 0)
		  dist += (fabs(f1[i]-f2[i])/aux);
  }

  return(dist);
}

// Compute  Squared Chord distance between feature vectors
float opf_SquaredChordDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f, aux1, aux2;

  for (i=0; i < n; i++){
	  aux1 = sqrtf(f1[i]);
	  aux2 = sqrtf(f2[i]);

	  if((aux1 >= 0) && (aux2 >=0))
		  dist += pow(aux1-aux2,2);
  }

  return(dist);
}

// Compute  Squared Chi-squared distance between feature vectors
float opf_SquaredChiSquaredDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f, aux;

  for (i=0; i < n; i++){
	  aux = fabs(f1[i]+f2[i]);
	  if(aux > 0)
		  dist += (pow(f1[i]-f2[i],2)/aux);
  }

  return(dist);
}

// Compute  Bray Curtis distance between feature vectors
float opf_BrayCurtisDist(float *f1, float *f2, int n){
  int i;
  float dist=0.0f, aux;

  for (i=0; i < n; i++){
	  aux = f1[i]+f2[i];
	  if(aux > 0)
		  dist += (fabs(f1[i]-f2[i])/aux);
  }

  return(dist);
}

/* -------- Auxiliary functions to optimize BestkMinCut -------- */

// Create adjacent list in subgraph: a knn graph.
// Returns an array with the maximum distances
// for each k=1,2,...,kmax
float* opf_CreateArcs2(Subgraph *sg, int kmax)
{
    int    j,l,k;
    float  dist;
    std::vector<int> nn(kmax + 1);
    std::vector<float> d(kmax + 1);
    float *maxdists=AllocFloatArray(kmax);
    /* Create graph with the knn-nearest neighbors */

    sg->df=0.0;
    for (size_t i=0; i < sg->nodes.size(); i++)
    {
        for (l=0; l < kmax; l++)
            d[l]=FLT_MAX;
        for (j=0; j < static_cast<int>(sg->nodes.size()); j++)
        {
            if (j!=static_cast<int>(i))
            {
                if(!opf_PrecomputedDistance)
                    d[kmax] = opf_ArcWeight(sg->nodes[i].feat,sg->nodes[j].feat,sg->nfeats);
                else
                    d[kmax] = opf_DistanceValue[sg->nodes[i].position][sg->nodes[j].position];
                nn[kmax]= j;
                k      = kmax;
                while ((k > 0)&&(d[k]<d[k-1]))
                {
                    dist    = d[k];
                    l       = nn[k];
                    d[k]    = d[k-1];
                    nn[k]   = nn[k-1];
                    d[k-1]  = dist;
                    nn[k-1] = l;
                    k--;
                }
            }
        }
        sg->nodes[i].radius = 0.0;
        sg->nodes[i].nplatadj = 0; //zeroing amount of nodes on plateaus
        //making sure that the adjacent nodes be sorted in non-decreasing order
        for (l=kmax-1; l >= 0; l--)
        {
            if (d[l]!=FLT_MAX)
            {
                if (d[l] > sg->df)
                    sg->df = d[l];
                if (d[l] > sg->nodes[i].radius)
                    sg->nodes[i].radius = d[l];
                if(d[l] > maxdists[l])
                    maxdists[l] = d[l];
                //adding the current neighbor at the beginnig of the list
                sg->nodes[i].adj.push_front(nn[l]);
            }
        }
    }

    if (sg->df<0.00001)
        sg->df = 1.0;

    return maxdists;
}

// OPFClustering computation only for sg->bestk neighbors
void opf_OPFClusteringToKmax(Subgraph *sg)
{
    int p, q, l, ki;
    const int kmax = sg->bestk;
    float tmp;
    std::vector<float> pathval(sg->nodes.size());
    RealHeap *Q = CreateRealHeap(sg->nodes.size(), pathval.data());
    SetRemovalPolicyRealHeap(Q, MAXVALUE);

    //   Add arcs to guarantee symmetry on plateaus
    for (size_t i=0; i < sg->nodes.size(); i++)
    {
        auto it = sg->nodes[i].adj.begin();
        ki = 1;
        while (ki <= kmax && it != sg->nodes[i].adj.end())
        {
            int j = *it;
            if (sg->nodes[i].dens==sg->nodes[j].dens)
            {
                // insert i in the adjacency of j if it is not there.
                bool found = false;
                auto it_j = sg->nodes[j].adj.begin();
                int kj = 1;
                while(kj <= kmax && it_j != sg->nodes[j].adj.end()){
                    if (static_cast<int>(i) == *it_j)
                    {
                        found=true;
                        break;
                    }
                    it_j++;
                    kj++;
                }

                if (!found)
                {
                    sg->nodes[j].adj.push_front(static_cast<int>(i)); // To be consistent with InsertSet logic
                    sg->nodes[j].nplatadj++; //number of adjacent nodes on
                                            //plateaus (includes adjacent plateau
                                            //nodes computed for previous kmax's)
                }
            }
            it++;
            ki++;
        }
    }

    // Compute clustering
    for (p = 0; p < static_cast<int>(sg->nodes.size()); p++)
    {
        pathval[p] = sg->nodes[p].pathval;
        sg->nodes[p].pred  = NIL;
        sg->nodes[p].root  = p;
        InsertRealHeap(Q, p);
    }

    l = 0;
    int i = 0;
    while (!IsEmptyRealHeap(Q))
    {
        RemoveRealHeap(Q,&p);
        sg->ordered_list_of_nodes[i]=p;
        i++;

        if ( sg->nodes[p].pred == NIL )
        {
            pathval[p] = sg->nodes[p].dens;
            sg->nodes[p].label=l;
            l++;
        }

        sg->nodes[p].pathval = pathval[p];
        const int nadj = sg->nodes[p].nplatadj + kmax; // total amount of neighbors
        
        auto it_p = sg->nodes[p].adj.begin();
        ki = 1;
        while(ki <= nadj && it_p != sg->nodes[p].adj.end())
        {
            q = *it_p;
            if ( Q->color[q] != BLACK )
            {
                tmp = MIN( pathval[p], sg->nodes[q].dens);
                if ( tmp > pathval[q] )
                {
                    UpdateRealHeap(Q,q,tmp);
                    sg->nodes[q].pred  = p;
                    sg->nodes[q].root  = sg->nodes[p].root;
                    sg->nodes[q].label = sg->nodes[p].label;
                }
            }
            it_p++;
            ki++;
        }
    }

    sg->nlabels = l;

    DestroyRealHeap( &Q );
}

// PDF computation only for sg->bestk neighbors
void opf_PDFtoKmax(Subgraph *sg)
{
    const int kmax = sg->bestk;
    double  dist;
    std::vector<float> value(sg->nodes.size());

    sg->K    = (2.0*(float)sg->df/9.0);

    sg->mindens = FLT_MAX;
    sg->maxdens = FLT_MIN;
    for (size_t i=0; i < sg->nodes.size(); i++)
    {
        value[i]=0.0;
        int nelems=1;
        
        auto it = sg->nodes[i].adj.begin();
        int k = 1;
        //the PDF is computed only for the kmax adjacents
        //because it is assumed that there will be no plateau
        //neighbors yet, i.e. nplatadj = 0 for every node in sg
        while (k <= kmax && it != sg->nodes[i].adj.end())
        {
            int neighbor = *it;
        	if(!opf_PrecomputedDistance)
            	dist  = opf_ArcWeight(sg->nodes[i].feat,sg->nodes[neighbor].feat,sg->nfeats);
			else
				dist = opf_DistanceValue[sg->nodes[i].position][sg->nodes[neighbor].position];
            value[i] += exp(-dist/sg->K);
            it++;
            k++;
            nelems++;
        }

        value[i] = (value[i]/(float)nelems);

        if (value[i] < sg->mindens)
            sg->mindens = value[i];
        if (value[i] > sg->maxdens)
            sg->maxdens = value[i];
    }

    if (sg->mindens==sg->maxdens)
    {
        for (size_t i=0; i < sg->nodes.size(); i++)
        {
            sg->nodes[i].dens = opf_MAXDENS;
            sg->nodes[i].pathval= opf_MAXDENS-1;
        }
    }
    else
    {
        for (size_t i=0; i < sg->nodes.size(); i++)
        {
            sg->nodes[i].dens = ((float)(opf_MAXDENS-1)*(value[i]-sg->mindens)/(float)(sg->maxdens-sg->mindens))+1.0;
            sg->nodes[i].pathval=sg->nodes[i].dens-1;
        }
    }
}


// Normalized cut computed only for sg->bestk neighbors
float opf_NormalizedCutToKmax( Subgraph *sg )
{
    int l, p, q, k;
    const int kmax = sg->bestk;
    float ncut, dist;
    std::vector<float> acumIC(sg->nlabels, 0.0f); //acumulate weights inside each class
    std::vector<float> acumEC(sg->nlabels, 0.0f); //acumulate weights between the class and a distinct one

    ncut = 0.0;

    for ( p = 0; p < static_cast<int>(sg->nodes.size()); p++ )
    {
        const int nadj = sg->nodes[p].nplatadj + kmax; //for plateaus the number of adjacent
                                                      //nodes will be greater than the current
                                                      //kmax, but they should be considered
        
        auto it = sg->nodes[p].adj.begin();
        k = 1;
        while(k <= nadj && it != sg->nodes[p].adj.end())
		{
            q = *it;
			if(!opf_PrecomputedDistance)
            	dist = opf_ArcWeight(sg->nodes[p].feat,sg->nodes[q].feat,sg->nfeats);
			else
	      		dist = opf_DistanceValue[sg->nodes[p].position][sg->nodes[q].position];
            if ( dist > 0.0 )
            {
                if ( sg->nodes[p].label == sg->nodes[q].label )
                {
                    acumIC[ sg->nodes[p].label ] += 1.0 / dist; // intra-class weight
                }
                else   // inter - class weight
                {
                    acumEC[ sg->nodes[p].label ] += 1.0 / dist; // inter-class weight
                }
            }
            it++;
            k++;
        }
    }

    for ( l = 0; l < static_cast<int>(sg->nlabels); l++ )
    {
        if ( acumIC[ l ] + acumEC[ l ]  > 0.0 ) ncut += (float) acumEC[ l ] / ( acumIC[ l ] + acumEC[ l ] );
    }
    return( ncut );
}
