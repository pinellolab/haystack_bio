rm -Rf precompiled_binary/Darwin/haystack*
rm precompiled_binary/Darwin/download_genome
pyinstaller haystack_pipeline.spec --clean --distpath  precompiled_binary/Darwin/ 
pyinstaller download_genome.spec --clean --distpath precompiled_binary/Darwin/ 
pyinstaller haystack_hotspots.darwin.spec --clean --distpath  precompiled_binary/Darwin/ 
pyinstaller haystack_motifs.spec --clean --distpath  precompiled_binary/Darwin/ 
pyinstaller haystack_tf_activity_plane.darwin.spec --clean --distpath  precompiled_binary/Darwin/
rm -Rf build
