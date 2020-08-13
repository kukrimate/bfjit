# bfjit
Optimizing compiler from brainfuck to AMD64. It turns a brainfuck input file
into AMD64 code in memory then executes it.

Code generation is done in two steps. First the brainfuck program is turned into
a custom IL, applying a few optimizations in the process. The IL tree is then
fed into the machine code generator which generates x86_64 code from the tree.

Perfomance testing was done using Erik Bosman's Mandelbrot set viewer on an
Intel i7-4700M. The results are the sum of compile and execution time averaged
across 3 runs.

<table border="1">
	<tr>
		<td>Compiler/interpreter type</td>
		<td>Performance</td>
	</tr>
	<tr>
		<td>Simple interpreter</td>
		<td>52s</td>
	</tr>
	<tr>
		<td>Jump pre-computing interpreter</td>
		<td>30s</td>
	</tr>
	<tr>
		<td>Brainfuck to C translator (-O0)</td>
		<td>0.2s + 3.5s</td>
	</tr>
	<tr>
		<td>Brainfuck to C translator (-O3)</td>
		<td>2s + 1s</td>
	</tr>
	<tr>
		<td>bfjit</td>
		<td>0.01s + 1s</td>
	</tr>
</table>

Currently bfjit only does optimizations on the Brainfuck tree, but further
optimizations are possible both in the Brainfuck and IL stages.
