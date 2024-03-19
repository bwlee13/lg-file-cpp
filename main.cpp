#include <iostream>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <tbb/parallel_for.h>
#include <tbb/combinable.h>


void ParLoop(const std::vector<std::shared_ptr<arrow::StringArray>>& columns, int16_t col_idx, int num_rows, tbb::combinable<int> &rolling_sum_combinable ) {
    const auto& column = columns[col_idx];
    const int32_t *offsets = column->raw_value_offsets();
    const uint8_t *values = column->value_data()->data();

    for (int row_idx = 0; row_idx < num_rows; ++row_idx) {
        int32_t start_offset = offsets[row_idx];
        int32_t end_offset = (row_idx == column->length() - 1) ? column->value_data()->size() : offsets[row_idx + 1];
        int32_t length = end_offset - start_offset;
        if (length == 1) {
            const uint8_t *raw_string = values + start_offset;
            if ( raw_string[0] == 0x61) {
                const auto &next_column = columns[col_idx + 1]; // Assuming safe access to col_idx + 1
                const int32_t *next_offsets = next_column->raw_value_offsets();
                const uint8_t *next_values = next_column->value_data()->data();

                const int32_t start_offset2 = next_offsets[row_idx];
                const uint8_t *raw_string2 = next_values + start_offset2;

                const int val = raw_string2[0] - '0';
                rolling_sum_combinable.local() += val;
            }

//                auto val = columns[col_idx + 1]->GetView(row_idx)[0] - '0';
//                rolling_sum_combinable.local() += val;
            }
        }
}

void ProcessParquetFile(const std::string &file_path) {
    auto infile_result = arrow::io::ReadableFile::Open(file_path, arrow::default_memory_pool());

    if (!infile_result.ok()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    std::shared_ptr<arrow::io::ReadableFile> infile = *infile_result;

    std::unique_ptr<parquet::arrow::FileReader> reader;
    auto open_file_status = parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader);

    if (!open_file_status.ok()) {
        std::cerr << "Failed to open Parquet file reader." << std::endl;
        return;
    }
    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(reader->ReadTable(&table));

    auto num_rows = table->num_rows();
    int16_t num_cols = 50;

    PARQUET_THROW_NOT_OK(table->CombineChunks(arrow::default_memory_pool()));

    tbb::combinable<int> rolling_sum_combinable(0);

    std::vector<std::shared_ptr<arrow::StringArray>> columns(num_cols);
    for (int16_t i = 0; i < num_cols; ++i) {
        columns[i] = std::static_pointer_cast<arrow::StringArray>(table->column(i)->chunk(0));
    }

    tbb::parallel_for(tbb::blocked_range<size_t>(0, num_cols-1), [&](const tbb::blocked_range<size_t>& range) {
        for(size_t col_idx = range.begin(); col_idx != range.end(); ++col_idx) {
            ParLoop(columns, col_idx, num_rows, rolling_sum_combinable);
        }
    });

    int rolling_sum = rolling_sum_combinable.combine(std::plus<int>());

    std::cout << "Total Sum: " << rolling_sum << std::endl;
}
int main() {

    auto start = std::chrono::high_resolution_clock::now();

    const std::string fp = "../test_dataset.parquet";
    ProcessParquetFile(fp);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Function took " << duration.count() << " milliseconds to execute.\n";

    return 0;
}
