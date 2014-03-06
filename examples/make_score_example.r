#!/usr/bin/env Rscript

##
## A short R script to generate random data for the 'scores' example.
##
library(randomNames)

sat_data = function(count,group_name,mean,sd)
{
  return(
     data.frame(Name=randomNames(count,gender="M",which.names="first"),
                TeachingMethod=rep(group_name,count),
                Score=round(rnorm(count, mean=mean,sd=sd)/10)*10))
}

exp = rbind( sat_data( runif(1,min=20,max=50), "Classroom", 500, 100 ),
	     sat_data( runif(1,min=20,max=50), "Online", 550, 110 ),
	     sat_data( runif(1,min=20,max=50), "PrepBook", 520, 90 ))

write.table(exp,file="scores.txt",sep="\t",row.names=FALSE,col.names=FALSE,quote=FALSE);
write.table(exp,file="scores_h.txt",sep="\t",row.names=FALSE,col.names=T,quote=FALSE);
