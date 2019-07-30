This is a basic public domain implementation of Picture Resembling codes (PR codes) using generalization of Kuznetsov-Tsybakov problem to weak (statistical) constraints - 2D barcodes like QR codes, chosen to resemble given picture.

It treats grayscale picture "picture.bmp" as statistical constraints: grayness g of pixel denotes that we should use 
Pr(1)=g probability distribution for corresponding pixel of the barcode (unknown to the receiver).
encode() takes first bytes of "data.dat" (as many as the size will allow) and creates code for it, which resemble "picture.bmp" - the output is written as "code.bmp"
decode() takes "code.bmp" and decodes hidden information to "out.dat" (does not use "picture.bmp")

The F parameter controls rate=1-F/N and contrast: the larger, the better contrast, the lower code capacity ("number of pixels" * "rate" bits).
F is the number of freedom bits per block - these bits can be freely chosen by encoder - getting 2^F possibilities to expand given node, among which we choose the best one for given constraints.
The encoder propagates at most maxactive=10000 best encoding for every position - the more, the slower and the closer to theoretical capacity.

It should work for most square(!) bitmaps of resolution <600 not divided by 19 or 29 (used for pixel ordering).
The library for .bmp I have used ( http://bitmap.codeplex.com/ ) requires that it is 24 bit, white/black are written as red=grgreen=blue=0 or 255.

This is a very basic version - feel free to expand it.
I plan to add error correction using implementation from https://indect-project.eu/correction-trees/
If there will be an interest, I can also add dithering: for example first set even pixels with lower freedom, then diffuse their errors to odd pixels and set them with higher freedom.

Preprint: http://arxiv.org/abs/1211.1572

Final 2015 paper: https://ieeexplore.ieee.org/document/7348695

Slides: https://www.dropbox.com/s/bhgy55i1wt5dram/qrsem.pdf

Here are compiled files and some examples of usage: https://www.dropbox.com/s/tjt1nwf1zsk3y7c/PRcodes.zip

Jarek Duda, Feb 2014
(2019: repaired links and added 2015 paper)
