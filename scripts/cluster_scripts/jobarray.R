marks <- c('H3K27ac', 'DNase', 'H3K4me3', 'H3K27me3', 'methylation')
output_root_dir <- "/data/pinello/PROJECTS/2017_06_HAYSTACK/precomputed_tracks"
genome <- "hg19"

output_dir <- "/mnt/hd2/DATA_PINELLO/PROJECTS/2017_06_HAYSTACK/precomputed_tracks/jobscripts"

filenames <- list()

for (mark in marks){
  
  filenames[[mark]] <- file.path(
    output_dir, 
    sprintf(
      "lsf_jobscript_%s.txt", 
      mark))
  
  fileConn <- file(filenames[[mark]])
  
  mark_dir <- file.path(output_root_dir, mark, sprintf("sample_names_%s.txt ", mark)) 
  samplefile <- file.path(output_root_dir, mark, "conda") 
  
  cmd <- paste0(sprintf("#BSUB -J %s \n", mark),
                sprintf("#BSUB -o log.%s \n", mark),
                "#BSUB -n 24 \n",
                "#BSUB -M 128000 \n",
                "#BSUB -q big-multi \n",
                "#BSUB -N \n \n",
                "source activate haystack_conda \n",
                "run_pipeline.py ",
                samplefile,
                genome,
                "--output_directory ",
                mark_dir, 
                " --input_is_bigwig --blacklist hg19 --n_processes 24  --temp_directory  /dev/shm ")
  
  if(mark=="methylation"){
    cmd <- paste0(cmd, "--depleted")
  }
  
  writeLines(cmd, fileConn)
  close(fileConn)
}

#####

filebash <- file(file.path(
  output_dir, 
  "lsf_submit.sh"),
  open="wt"
)

writeLines("#!/bin/bash", filebash)

for (mark in marks){
  writeLines(paste0("bsub < ", basename(filenames[[mark]])), filebash)
}

close(filebash )
