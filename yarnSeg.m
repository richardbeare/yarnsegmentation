function yarnSeg()

filt=imread('PicsForDorian/filtered_img.png');
skel=imread('PicsForDorian/skeleton_img.png');

% stage 1 - find markers for bright regions
mod1=imimposemin(filt, skel);
ws1=watershed(mod1)==0;
imwrite(ws1, 'ws1.tif');

% compute a stupid gradient 
hy = fspecial('sobel');
hx = hy';
Iy = imfilter(double(filt), hy, 'replicate');
Ix = imfilter(double(filt), hx, 'replicate');
gradmag = sqrt(Ix.^2 + Iy.^2);
% smooth it - may not be needed
H = fspecial('gaussian',5,1);
gradmag=imfilter(gradmag, H, 'replicate');
% impose minima again
imwrite(skel | ws1, 'mark.tif')
mod2=imimposemin(gradmag, skel | ws1);

seg=watershed(mod2);
stats=regionprops(seg, skel, 'MaxIntensity');

% this almost works, but some of the markers touch. This is not a problem
% if we get to create the markers ourselves, but the imposemin approach
% doesn't let us do that. Therefore touching markers become one component
% and join neighbouring regions.
keep=find([stats.MaxIntensity] > 0);
final=ismember(seg, keep);
imwrite(final, 'seg.tif');
end