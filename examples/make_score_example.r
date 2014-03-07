#!/usr/bin/env Rscript

##
## A short R script to generate random data for the 'scores' example.
##
library(randomNames)

gen_data = function(count,group_name,mean,sd)
{
  return(
     data.frame(Name=gsub(" ","-",randomNames(count,gender="M",which.names="first")),
                Major=rep(group_name,count),
                Score=pmin(round(rnorm(count, mean=mean,sd=sd)),100)))
}

exp = rbind(
	gen_data( runif(1,min=10,max=20), "Arts", runif(1,min=50,max=90), runif(1,min=5,max=20) ),
	gen_data( runif(1,min=10,max=20), "Business", runif(1,min=50,max=90), runif(1,min=5,max=20) ),
	gen_data( runif(1,min=10,max=20), "Health-Medicine", runif(1,min=50,max=90), runif(1,min=5,max=20) ),
	gen_data( runif(1,min=10,max=20), "Social-Sciences", runif(1,min=50,max=90), runif(1,min=5,max=20) ),
	gen_data( runif(1,min=10,max=20), "Life-Sciences", runif(1,min=50,max=90), runif(1,min=5,max=20) ),
	gen_data( runif(1,min=10,max=20), "Engineering", runif(1,min=50,max=90), runif(1,min=5,max=20) )
       )

write.table(exp,file="scores.txt",sep="\t",row.names=FALSE,col.names=FALSE,quote=FALSE);
write.table(exp,file="scores_h.txt",sep="\t",row.names=FALSE,col.names=T,quote=FALSE);
