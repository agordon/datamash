#!/usr/bin/env Rscript

## Copyright (C) 2014-2020 Assaf Gordon <assafgordon@gmail.com>
##
## This file is part of Compute.
##
## Compute is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Compute is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Compute.  If not, see <https://www.gnu.org/licenses/>.

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
