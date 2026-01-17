%% DDS Complete Signal Flow Visualization - 3x3 Grid Layout
% Publication-quality figure for top-tier journals
% System Clock: 24 MHz, Output: 1 MHz
% N = 28 bits Phase Accumulator
% Columns: Sine Wave | Square Wave | Triangle Wave
% Rows: Phase Accumulator | DAC Output | LPF Output

clear; clc; close all;

%% ==================== Publication Settings ====================
set(0, 'DefaultAxesFontName', 'Times New Roman');
set(0, 'DefaultAxesFontSize', 10);
set(0, 'DefaultTextFontName', 'Times New Roman');
set(0, 'DefaultLineLineWidth', 1.5);

% IEEE/Nature color palette
c.blue = [0, 114, 189]/255;
c.orange = [217, 83, 25]/255;
c.green = [32, 134, 48]/255;
c.red = [162, 20, 47]/255;
c.purple = [126, 47, 142]/255;
c.gray = [128, 128, 128]/255;
c.lightgray = [180, 180, 180]/255;

%% ==================== DDS Parameters ====================
N = 28;                          % Phase accumulator bits (28-bit)
f_clk = 24e6;                    % System clock: 24 MHz
f_out = 1e6;                     % Output frequency: 1 MHz
FTW = round(f_out * 2^N / f_clk); % Frequency tuning word

% LUT parameters
LUT_bits = 12;
LUT_size = 2^LUT_bits;

% Generate LUTs
lut_addr = 0:LUT_size-1;
lut_sine = sin(2*pi*lut_addr/LUT_size);
lut_square = sign(sin(2*pi*lut_addr/LUT_size));
lut_square(lut_square == 0) = 1;
lut_triangle = 2*abs(2*lut_addr/LUT_size - 1) - 1;

% Simulation: 3 complete cycles with high resolution
num_cycles = 3;
samples_per_cycle = round(f_clk / f_out);  % 24 samples per cycle
num_samples = num_cycles * samples_per_cycle + 1;
t = (0:num_samples-1) / f_clk;
t_us = t * 1e6;

% Phase accumulation
phase_acc = zeros(1, num_samples);
for i = 2:num_samples
    phase_acc(i) = mod(phase_acc(i-1) + FTW, 2^N);
end
phase_norm = phase_acc / 2^N;

% LUT address (use high bits of phase accumulator)
lut_address = floor(phase_acc / 2^(N-LUT_bits));
lut_address = min(lut_address, LUT_size-1);

% Generate waveform outputs from LUT
out_sine = zeros(1, num_samples);
out_square = zeros(1, num_samples);
out_triangle = zeros(1, num_samples);

for i = 1:num_samples
    addr = lut_address(i) + 1;
    out_sine(i) = lut_sine(addr);
    out_square(i) = lut_square(addr);
    out_triangle(i) = lut_triangle(addr);
end

%% ==================== LPF Design ====================
% Different cutoff frequencies for different waveforms
% Sine: just remove high-freq noise, keep sine shape
% Square: slightly smooth edges but keep square shape
% Triangle: keep triangle shape

% For sine wave - gentle LPF
fc_sine = f_out * 3;
[b_sine, a_sine] = butter(4, fc_sine/(f_clk/2));

% For square wave - very gentle LPF to keep edges
fc_square = f_out * 8;
[b_square, a_square] = butter(2, fc_square/(f_clk/2));

% For triangle wave - gentle LPF
fc_tri = f_out * 5;
[b_tri, a_tri] = butter(3, fc_tri/(f_clk/2));

% Apply filters
filtered_sine = filtfilt(b_sine, a_sine, out_sine);
filtered_square = filtfilt(b_square, a_square, out_square);
filtered_triangle = filtfilt(b_tri, a_tri, out_triangle);

% Ideal waveforms for reference
t_ideal = linspace(0, max(t_us), 2000);
ideal_sine = sin(2*pi*f_out*t_ideal/1e6);
ideal_square = sign(sin(2*pi*f_out*t_ideal/1e6));
ideal_tri = sawtooth(2*pi*f_out*t_ideal/1e6, 0.5);

%% ==================== Create 3x3 Figure ====================
fig = figure('Units', 'centimeters', 'Position', [1, 1, 24, 18], ...
    'Color', 'w', 'PaperPositionMode', 'auto');

% Column titles
col_titles = {'Sine Wave', 'Square Wave', 'Triangle Wave'};

% Colors for each waveform
wave_colors = {c.blue, c.green, c.red};

% Data arrays
dac_outputs = {out_sine, out_square, out_triangle};
lpf_outputs = {filtered_sine, filtered_square, filtered_triangle};
ideal_waves = {ideal_sine, ideal_square, ideal_tri};

%% -------------------- Plot 3x3 Grid --------------------
for col = 1:3
    for row = 1:3
        subplot(3, 3, (row-1)*3 + col);
        
        if row == 1
            % Row 1: Phase Accumulator (same for all waveforms)
            plot(t_us, phase_norm, 'Color', c.orange, 'LineWidth', 1.5);
            hold on;
            plot(t_us, phase_norm, '.', 'Color', c.orange, 'MarkerSize', 4);
            ylim([-0.05, 1.1]);
            set(gca, 'YTick', [0, 0.5, 1]);
            
            if col == 1
                ylabel({'(a) Phase'; 'Accumulator'}, 'FontSize', 10, 'FontWeight', 'bold');
            end
            
        elseif row == 2
            % Row 2: DAC Output (Staircase)
            plot(t_ideal, ideal_waves{col}, '--', 'Color', c.lightgray, 'LineWidth', 1);
            hold on;
            stairs(t_us, dac_outputs{col}, 'Color', wave_colors{col}, 'LineWidth', 1.2);
            plot(t_us, dac_outputs{col}, '.', 'Color', wave_colors{col}, 'MarkerSize', 4);
            ylim([-1.25, 1.4]);
            set(gca, 'YTick', [-1, 0, 1]);
            
            if col == 1
                ylabel({'(b) DAC Output'; '(Staircase)'}, 'FontSize', 10, 'FontWeight', 'bold');
            end
            if col == 3
                legend({'Ideal', 'DAC'}, 'Location', 'northeast', 'FontSize', 8, 'Box', 'off');
            end
            
        else
            % Row 3: LPF Output (Filtered) - should match waveform type
            plot(t_ideal, ideal_waves{col}, '--', 'Color', c.lightgray, 'LineWidth', 1);
            hold on;
            plot(t_us, lpf_outputs{col}, 'Color', wave_colors{col}, 'LineWidth', 1.8);
            ylim([-1.25, 1.4]);
            set(gca, 'YTick', [-1, 0, 1]);
            xlabel('Time (\mus)', 'FontSize', 10);
            
            if col == 1
                ylabel({'(c) LPF Output'; '(Filtered)'}, 'FontSize', 10, 'FontWeight', 'bold');
            end
            if col == 3
                legend({'Ideal', 'Filtered'}, 'Location', 'northeast', 'FontSize', 8, 'Box', 'off');
            end
        end
        
        % Common settings
        xlim([0, max(t_us)]);
        set(gca, 'TickDir', 'out', 'LineWidth', 1, 'FontSize', 9);
        box on;
        grid on;
        set(gca, 'GridLineStyle', ':', 'GridAlpha', 0.4);
        
        % Remove x-axis labels for rows 1-2
        if row < 3
            set(gca, 'XTickLabel', []);
        end
        
        % Add column title on top row
        if row == 1
            title(col_titles{col}, 'FontSize', 11, 'FontWeight', 'bold', 'Color', wave_colors{col});
        end
    end
end

%% ==================== Add Main Title ====================
annotation('textbox', [0.0, 0.955, 1.0, 0.045], 'String', ...
    sprintf('DDS Waveform Generation (f_{clk} = 24 MHz, f_{out} = 1 MHz, N = %d bits, FTW = %d)', N, FTW), ...
    'FontSize', 12, 'FontWeight', 'bold', 'FontName', 'Times New Roman', ...
    'EdgeColor', 'none', 'HorizontalAlignment', 'center', 'Interpreter', 'tex');

%% ==================== Adjust Subplot Spacing ====================
% Tighten the layout
set(gcf, 'Units', 'normalized');
drawnow;

%% ==================== Save Figure ====================
print(gcf, 'DDS_3x3_Waveforms', '-dpng', '-r600');
print(gcf, 'DDS_3x3_Waveforms', '-depsc2');
print(gcf, 'DDS_3x3_Waveforms', '-dpdf');

fprintf('\n============ DDS 3x3 Waveform Visualization ============\n');
fprintf('System Parameters:\n');
fprintf('  System Clock (f_clk): 24 MHz\n');
fprintf('  Output Frequency (f_out): 1 MHz\n');
fprintf('  Phase Accumulator Bits (N): %d bits\n', N);
fprintf('  Frequency Tuning Word (FTW): %d\n', FTW);
fprintf('  LUT Size: %d entries\n', LUT_size);
fprintf('  Samples per Cycle: %d\n', samples_per_cycle);
fprintf('  Frequency Resolution: %.6f Hz\n', f_clk/2^N);
fprintf('\nLayout:\n');
fprintf('  Row 1: Phase Accumulator (sawtooth)\n');
fprintf('  Row 2: DAC Output (staircase with samples)\n');
fprintf('  Row 3: LPF Output (filtered waveforms)\n');
fprintf('  Col 1: Sine Wave | Col 2: Square Wave | Col 3: Triangle Wave\n');
fprintf('\nSaved: DDS_3x3_Waveforms.png, .eps, .pdf (600 DPI)\n');
fprintf('========================================================\n');
