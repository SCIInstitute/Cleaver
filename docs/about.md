---
layout: default
title: Main Page
sciurl: https://www.sci.utah.edu/software/cleaver.html
releaseurl: https://github.com/SCIInstitute/Cleaver2/releases
project: Cleaver2
---

## Cleaver2

[Cleaver2](https://github.com/SCIInstitute/Cleaver2/releases) is a free multimaterial tetrahedral meshing tool developed by the NIH Center for Integrative Biomedical Computing at the University of Utah Scientific Computing and Imaging (SCI) Institute.

### Installation

Check the [Platform Specifications](#specs.html) for system requirements.

[Installers](https://github.com/SCIInstitute/Cleaver2/releases) are provided for Windows and Mac OS X. Linux users need to build Cleaver2 from [source](#build.html)

### Documentation
<ul>
  {% for page in site.pages %}
  {% if page.category == "info" %}
  <h3> <a href={{ site.github.url }}{{ page.url }}>{{ page.title }}</a></h3>
  {% endif %}
  {% endfor %}
  <h3><a href={{ site.github.url }}/doxygen/index.html>Doxygen Code Reference</a></h3>
</ul>
