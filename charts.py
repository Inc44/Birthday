import matplotlib.pyplot as plt


def extract_data_entries(file_path):
	extracted_data_entries = []
	with open(file_path, "r", encoding="utf-8") as file:
		lines = file.readlines()
		for i in range(0, len(lines), 5):
			command_line = lines[i + 1].strip()
			execution_time = float(lines[i + 3].split(":")[1].strip().split()[0])
			extracted_data_entries.append(
				{"Command": command_line, "Execution Time (s)": execution_time}
			)
	extracted_data_entries.sort(key=lambda x: x["Execution Time (s)"])
	return extracted_data_entries


def plot_execution_times(commands, times, title, file_name):
	plt.figure(figsize=(16, 12))
	plt.plot(commands, times, marker="o", linestyle="-", color="blue")
	plt.title(title)
	plt.xlabel("Command")
	plt.ylabel("Execution Time (s)")
	plt.yscale("log")
	plt.xticks(rotation=45, ha="right")
	plt.grid(True, which="both", ls="--")
	plt.subplots_adjust(left=0.07, bottom=0.27, right=0.93, top=0.93)
	plt.savefig(file_name, dpi=100, bbox_inches="tight", pad_inches=0.1)
	plt.close()


file_path = "1_000_000"
data_entries = extract_data_entries(file_path)
commands_extracted = [entry["Command"] for entry in data_entries]
execution_times_extracted = [entry["Execution Time (s)"] for entry in data_entries]
plot_title = "Execution Time by Command for 1 000 000 Dataset Size (Log Scale)"
output_file_name = "1_000_000.png"
plot_execution_times(
	commands_extracted, execution_times_extracted, plot_title, output_file_name
)
file_path = "10_000_000"
data_entries = extract_data_entries(file_path)
commands_extracted = [entry["Command"] for entry in data_entries]
execution_times_extracted = [entry["Execution Time (s)"] for entry in data_entries]
plot_title = "Execution Time by Command for 10 000 000 Dataset Size (Log Scale)"
output_file_name = "10_000_000.png"
plot_execution_times(
	commands_extracted, execution_times_extracted, plot_title, output_file_name
)
file_path = "100_000_000"
data_entries = extract_data_entries(file_path)
commands_extracted = [entry["Command"] for entry in data_entries]
execution_times_extracted = [entry["Execution Time (s)"] for entry in data_entries]
plot_title = "Execution Time by Command for 100 000 000 Dataset Size (Log Scale)"
output_file_name = "100_000_000.png"
plot_execution_times(
	commands_extracted, execution_times_extracted, plot_title, output_file_name
)
file_path = "1_000_000_000"
data_entries = extract_data_entries(file_path)
commands_extracted = [entry["Command"] for entry in data_entries]
execution_times_extracted = [entry["Execution Time (s)"] for entry in data_entries]
plot_title = "Execution Time by Command for 1 000 000 000 Dataset Size (Log Scale)"
output_file_name = "1_000_000_000.png"
plot_execution_times(
	commands_extracted, execution_times_extracted, plot_title, output_file_name
)
