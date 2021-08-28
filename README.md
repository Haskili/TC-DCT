<h1 align="center">TC-DCT</h1> 
  <p align="center">
  <img src="https://i.imgur.com/zB2YVNx.png"  alt="blink-icon"  width="100"  height="70"><br>
    An analysis on the threaded C implementation of <br> the Discrete Cosine Transform (DCT)
    <br/><br/><br/>
    [<a href="https://en.wikipedia.org/wiki/Discrete_cosine_transform">Discrete Cosine Transform</a>]
    [<a href="https://github.com/Haskili/TC-DCT#acknowledgements">Acknowledgements</a>]
    [<a href="https://github.com/Haskili/TC-DCT/issues">Issues</a>]
  </p>
</p>

## Overview

This project is my submission for the final project in CSCI-551, *Parallel Programming and Numerical Methods*, as California State University, Chico.
<br>

It serves as an implementation of multithreading the Discrete Cosine Transform (DCT) algorithm; as well as an analysis into how well that implementation performed under rigerous testing and observation. This anaylsis is oriented towards the questions required by the assignment, and is by no means or method a full anaylsis into the algorithm or my implementation of it. However, it does cover basics such as how well it scales, it's performance with regards to Amdahl's law, etc.
<br>

I hold no responsibility for what you choose to do with this information, but ask that you please act responsibly.
<br></br>

## Process
**Step-1: Obtaining Source Input**

**Step-2: Threading Setup**

**Step-3: Threading Runtime**

**Step-4: Collecting & Verifying Results**

**Final Result**
As you can see here, it works flawlessly, and has no measurable errors in the image result.
When verifying

## Usage


## Performance Measuring


## Acknowledgements
*TC-DCT* was originally meant as a school project. As such, it is heavily commented and there may be certain sections with verbose documentation.

A good portion of the comments for `SSIM()` & `PSNR()` refer to the formulas, most if not all of which can be found in the two Wikipedia pages below:
* [SSIM Wikipedia page](https://en.wikipedia.org/wiki/Structural_similarity)
* [PSNR Wikipedia page](https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio)

