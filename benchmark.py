import re
import csv
import subprocess
from pathlib import Path
from typing import List

COMPILERS = (
	"gcc",
	"g++",
	"zig",
	"nvcc",
	"go",
	"python",
	"cargo",
	"clang",
	"clang++",
	"csc",
	"mono",
	"javac",
	"java",
	"node",
	"lua",
	"ocamlopt",
)
SOURCES = [
	"birthday.c",
	"birthday.cu",
	"birthday.go",
	"birthday.py",
	"birthday_numpy.py",
	"src/main.rs",
	"birthday_opencl.c",
	"birthday_vulkan.c",
	"birthday.cpp",
	"Birthday.cs",
	"Birthday.java",
	"birthday.js",
	"birthday.lua",
	"birthday.ml",
	"birthday.zig",
]
NUM_RUNS = 4
TIMEOUT = 60
TOTAL_SIMULATIONS = [1_000_000, 10_000_000, 100_000_000, 1_000_000_000]
TOTAL_SIMULATIONS_RE = [
	r"(TOTAL_SIMULATIONS\s*=\s*)(\d[0-9_]*)(\b)",  # C/CUDA/OpenCL/Vulkan/C++
	r"(const\s+totalSimulations\s*=\s*)(\d[0-9_]*)(\b)",  # Go
	r"(TOTAL_SIMULATIONS\s*:\s*Final\s*\[\s*int\s*\]\s*=\s*)(\d[0-9_]*)(\b)",  # Python
	r"(const\s+TOTAL_SIMULATIONS\s*:\s*u32\s*=\s*)(\d[0-9_]*)(;)",  # Rust, Zig
	r"(const\s+int\s+TOTAL_SIMULATIONS\s*=\s*)(\d[0-9_]*)(;)",  # C#
	r"(static\s+final\s+int\s+TOTAL_SIMULATIONS\s*=\s*)(\d[0-9_]*)(;)",  # Java
	r"(const\s+TOTAL_SIMULATIONS\s*=\s*)(\d[0-9_]*)(;)",  # JS
	r"(local\s+TOTAL_SIMULATIONS\s*=\s*)(\d[0-9_]*)(\b)",  # Lua
	r"(let\s+total_simulations\s*=\s*)(\d[0-9_]*)(\b)",  # OCaml
]
TOTAL_SIMULATIONS_RE_COMP = [
	re.compile(pattern, flags=re.MULTILINE) for pattern in TOTAL_SIMULATIONS_RE
]
PROBABILITY_RE = r"Probability:\s*([0-9]*\.?[0-9]+)"
EXECUTION_TIME_RE = r"Execution Time:\s*([0-9]*\.?[0-9]+)"
PROBABILITY_RE_COMP = re.compile(PROBABILITY_RE)
EXECUTION_TIME_RE_COMP = re.compile(EXECUTION_TIME_RE)


def read_text(path: Path) -> str:
	with open(path, "r", encoding="utf-8") as file:
		return file.read()


def write_text(path: Path, text: str):
	with open(path, "w", encoding="utf-8") as file:
		file.write(text)


def run_command(cmd: str, timeout: float) -> tuple[int, str, str]:
	try:
		resp = subprocess.run(
			cmd, shell=True, capture_output=True, text=True, timeout=timeout
		)
		return resp.returncode, resp.stdout, resp.stderr
	except subprocess.TimeoutExpired:
		return 124, "", f"Timeout after {timeout}s"


def parse_output(stdout: str, stderr: str) -> tuple[float, float] | tuple[None, None]:
	text = stdout + ("\n" + stderr if stderr else "")
	probability_match = PROBABILITY_RE_COMP.search(text)
	execution_time_match = EXECUTION_TIME_RE_COMP.search(text)
	if not probability_match or not execution_time_match:
		return None, None
	probability = float(probability_match.group(1))
	execution_time = float(execution_time_match.group(1))
	if not (0.0 <= probability <= 1.0):
		return None, None
	return probability, execution_time


def replace_total_simulations(path: Path, simulations: int) -> bool:
	text = read_text(path)
	replaced = False
	for comp in TOTAL_SIMULATIONS_RE_COMP:
		target = comp.sub(
			lambda m: m.group(1)
			+ str(simulations)
			+ (m.group(3) if m.lastindex and m.lastindex >= 3 else ""),
			text,
		)
		if target != text:
			text = target
			replaced = True
	if replaced:
		write_text(path, text)
	return replaced


def write_csv(path: Path, table: List[dict]) -> None:
	with open(path, "w", encoding="utf-8", newline="") as file:
		writer = csv.writer(file)
		writer.writerow(
			[
				"source",
				"compile",
				"total_simulations",
				"probability",
				"execution_time",
				"num_runs",
			]
		)
		for row in table:
			writer.writerow(
				[
					row["source"],
					row["compile"],
					row["total_simulations"],
					f"{row['probability']:.9f}",
					f"{row['execution_time']:.3f}",
					row["num_runs"],
				]
			)


def write_markdown(path: Path, table: List[dict]) -> None:
	with open(path, "w", encoding="utf-8") as file:
		file.write(
			"| Source | Compiler | Total Simulations | Probability | Execution Time | Num Runs |\n"
		)
		file.write("|---|---|---|---|---|---|\n")
		for row in table:
			file.write(
				f"| {row['source']} | {row['compile']} | {row['total_simulations']} | {row['probability']:.9f} | {row['execution_time']:.3f} | {row['num_runs']} |\n"
			)


def write_benchmark(table: List[dict]) -> None:
	total_simulations = {
		1: 1_000_000,
		10: 10_000_000,
		100: 100_000_000,
		1000: 1_000_000_000,
	}
	buffers = {simulations: [] for simulations in total_simulations.values()}
	for row in table:
		simulations = total_simulations.get(int(row.get("total_simulations", 0)))
		if not simulations:
			continue
		buffers[simulations].append(f"{simulations:,}".replace(",", " "))
		buffers[simulations].append(row["compile"])
		buffers[simulations].append(f"Probability: {row['probability']:.9f}")
		buffers[simulations].append(f"Execution Time: {row['execution_time']:.3f} s")
		buffers[simulations].append("")
	for simulations, lines in buffers.items():
		text = ("\n".join(lines).strip() + "\n") if lines else ""
		path = f"{simulations:,}".replace(",", "_")
		with open(path, "w", encoding="utf-8") as file:
			file.write(text)


def extract_commands(lines: List[str], ext: str) -> List[str]:
	commands = []
	for i in range(len(lines)):
		line = lines[i].strip()
		if not line:
			continue
		if ext == ".py":
			if line.startswith("#"):
				payload = line[1:].strip()
				if payload:
					commands.append(payload)
				continue
			break
		if ext == ".ml":
			if line.startswith("(*") and "*)" in line:
				payload = line[2 : line.index("*)")].strip()
				if payload:
					commands.append(payload)
				continue
			break
		if ext == ".lua":
			if line.startswith("--"):
				payload = line[2:].strip()
				if payload:
					commands.append(payload)
				continue
			break
		if line.startswith("//"):
			payload = line[2:].strip()
			if payload:
				commands.append(payload)
			continue
		break
	return commands


def reconstruct_commands(commands: List[str]) -> List[str]:
	reconstructed_commands = []
	reconstructed_cmd = ""
	for cmd in commands:
		if reconstructed_cmd:
			reconstructed_cmd = (reconstructed_cmd + " " + cmd).strip()
			if not reconstructed_cmd.endswith("&&"):
				reconstructed_commands.append(reconstructed_cmd)
				reconstructed_cmd = ""
			continue
		if cmd.endswith("&&"):
			reconstructed_cmd = cmd
		else:
			reconstructed_commands.append(cmd)
	if reconstructed_cmd:
		reconstructed_commands.append(reconstructed_cmd)
	return reconstructed_commands


def filter_commands_pairs(
	reconstructed_commands: List[str],
) -> list[tuple[str | None, str]]:
	commands_pairs = []
	seen_commands = set()
	for cmd in reconstructed_commands:
		if not cmd:
			continue
		tokens = cmd.split()
		if not tokens or tokens[0] not in COMPILERS:
			continue
		if "&&" in cmd:
			parts = [part.strip() for part in cmd.split("&&") if part.strip()]
			if len(parts) >= 2:
				compile_cmd = " && ".join(parts[:-1])
				run_cmd = parts[-1]
			else:
				compile_cmd = None
				run_cmd = cmd.replace("&&", "").strip()
		else:
			compile_cmd = None
			run_cmd = cmd
		pair_cmd = (compile_cmd, run_cmd)
		if pair_cmd in seen_commands:
			continue
		seen_commands.add(pair_cmd)
		commands_pairs.append(pair_cmd)
	return commands_pairs


def extract_compiler(path: Path) -> list[tuple[str | None, str]]:
	ext = path.suffix.lower()
	lines = read_text(path).splitlines()[:16]
	commands = extract_commands(lines, ext)
	reconstructed = reconstruct_commands(commands)
	return filter_commands_pairs(reconstructed)


def main():
	table = []
	for i in SOURCES:
		path = Path(i)
		if not path.exists():
			continue
		compilers = extract_compiler(path)
		if not compilers:
			continue
		source = read_text(path)
		try:
			for simulations in TOTAL_SIMULATIONS:
				replace_total_simulations(path, simulations)
				for compile_cmd, run_cmd in compilers:
					if compile_cmd:
						returncode, _, _ = run_command(compile_cmd, TIMEOUT)
						if returncode != 0:
							continue
					probabilities = []
					execution_times = []
					ok = True
					for _ in range(NUM_RUNS):
						returncode, stdout, stderr = run_command(run_cmd, TIMEOUT)
						if returncode != 0:
							ok = False
							break
						probability, execution_time = parse_output(stdout, stderr)
						if probability is None or execution_time is None:
							ok = False
							break
						probabilities.append(probability)
						execution_times.append(execution_time)
					if ok and execution_times:
						table.append(
							{
								"source": str(path),
								"compile": compile_cmd + " && " + run_cmd
								if compile_cmd
								else run_cmd,
								"total_simulations": simulations // 1_000_000,
								"probability": sum(probabilities) / len(probabilities),
								"execution_time": sum(execution_times)
								/ len(execution_times),
								"num_runs": len(execution_times),
							}
						)
		finally:
			write_text(path, source)
	write_csv(Path("benchmark.csv"), table)
	write_markdown(Path("benchmark.md"), table)
	write_benchmark(table)


if __name__ == "__main__":
	main()
