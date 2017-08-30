%% figure 1: chinook salmon bycatch in the BSAI pollock fishery

bycatch = [38791 2114; 25691 10259; 17264 21252; 28451 4686; 10579 4405; ...
    36068 19554; 10935 33973; 15193 36130; 6352 5627; 3422 1539; ...
    18484 14961; 21794 12701; 32609 12977; 23093 28603; 27331 40030; ...
    58391 24305; 69409 52349; 16647 4853; 9688 2736; 7651 2084];
years = 1991:2010;

f1 = figure(1);
set(f1, 'Position', [60 684 600 400]);

bar(years, bycatch, 'stacked');
colormap([0.6 0.6 0.6; 0.9 0.9 0.9]);

axis([1990.5 2010.5 0 140000]);
set(gca, 'XTick', 1991:3:2010);
set(gca, 'YTick', 0:20000:140000);
set(gca, 'YTickLabel', {'0', '20', '40', '60', '80', '100', '120', '140'});
set(gca, 'TickDir', 'out');
box off;

xlabel('Year', 'FontSize', 12);
ylabel('Chinook salmon (thousands)', 'FontSize', 12);
legend('A season', 'B season', 'Location', 'NorthWest');
set(gca, 'FontSize', 11);

hgexport(f1, 'figure_1.eps');

%% figure 2: number of vessels that run out of credits 
% no trading, no reallocation, 47591 hardcap

years = 2000:2007;
vals = -ones(366, 8);

for i = 1:8
    filename = ['no_trading.no_realloc.47591.' num2str(years(i)) '.csv'];
    fid = fopen(filename);
    data = textscan(fid, '%s %d %*f %*f %*d %*d %*f', 'Delimiter', ',', 'Headerlines', 1);
    fclose(fid);
    dates = datenum(datevec(data{1}, 'mmm-dd-yyyy')) - datenum(years(i),1,1) + 1;
    vessels_done = data{2};
    
    if(mod(years(i), 4) == 0)
        feb_29 = find(dates == 60);
        vessels_done(feb_29:end-1) = vessels_done(feb_29+1:end);
    end
    vals(dates,i) = vessels_done;  
end

[r, c] = find(vals > -1);
days = 1:365;
first = min(r);
last = max(r);
vessels_out = vals(first:last,:);
days = days(first:last);
for i = 1:8
    vessels_out(1,i) = 0;
    negs = find(vessels_out(:,i) == -1);
    vessels_out_end = vessels_out(min(negs)-1,i);
    vessels_out(negs,i) = vessels_out_end;
end

a_season = 1:142;
b_season = 143:size(vessels_out,1);
line_formats = {'--', '-', '--', '-', '--', '-', '--', '-'};
line_colors = [repmat(0.75, 2, 3); repmat(0.25, 2, 3); repmat(0.5, 2, 3); repmat(0, 2, 3)];

f2 = figure(2);
close(f2);
f2 = figure(2);
set(f2, 'Position', [110 634 600 400]);

hold off;
for i = 1:8
    plot([days(a_season) days(142) days(b_season)], [vessels_out(a_season, i); NaN; vessels_out(b_season, i)], line_formats{i}, 'Color', line_colors(i,:), 'LineWidth', 1.2);
    hold on;
end

x_days = 2:35:285;
set(gca, 'XTick', x_days+19);
set(gca, 'XTickLabel', datestr(days(x_days)+366, 'mmm dd'));
%axis([20 304 -1 59]);
axis tight;
xlabel('Day', 'FontSize', 12);
ylabel('Number of vessels without ITEC', 'FontSize', 12);
legend('2000', '2001', '2002', '2003', '2004', '2005', '2006', '2007', 'Location', 'North');
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;

hgexport(f2, 'figure_2.eps');

%% figure 3: revenue losses
% (47,591) no reallocation, no trading, no avoidance incentives
% assume $0.20/lb for A season and $0.12/lb for B season

fid = fopen('no_trading.no_realloc.47591.unfished.csv');
data = textscan(fid, '%d %d %d', 'Delimiter', ',', 'Headerlines', 1);
fclose(fid);

a_value = 0.2*2204.62262;
b_value = 0.12*2204.62262;
years = data{1};
unfished = data{2} + data{3};
unfished_value = data{2} * a_value + data{3} * b_value;

f3 = figure(3);
set(f3, 'Position', [160 584-50 600 450]);
bar(years, unfished_value, 0.6);
colormap([0.6 0.6 0.6]);
xlabel('Year', 'FontSize', 12);
ylabel('Value of unfished walleye pollock (in millions USD)', 'FontSize', 12);
set(gca, 'FontSize', 11);
axis([1999.5 2007.5 0 100000000]);
set(gca, 'YTick', 0:10000000:100000000);
set(gca, 'YtickLabel', {'0', '10', '20', '30', '40', '50', '60', '70', '80', '90', '100'});
set(gca, 'TickDir', 'out');
box off;

hgexport(f3, 'figure_3.eps');

%% figure 4: smaller vessels show higher variability in bycatch rates

fid = fopen('sample_size_data.txt');
data = textscan(fid, '%f %f %f %f %f %f %f', 'Headerlines', 1);
fclose(fid);

raw_vals = [data{2} data{3} data{4} data{5} data{6}];
sd = std(raw_vals')';

f4 = figure(4);
set(f4, 'Position', [210 534 600 400]);
hold off;
plot(data{1}, data{7}, '-', 'Color', [0.5 0.5 0.5], 'LineWidth', 1.2);
hold on;
plot(data{1}, sd, 'd', 'Color', [0.2 0.2 0.2], 'MarkerFaceColor', [0.2 0.2 0.2]);
axis([0 6 0 1.4]);
xlabel('Walleye pollock catch share (%)', 'FontSize', 12);
ylabel('Standard deviation in relative bycatch rate', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;
legend('Observed', 'Modeled', 'Location', 'NorthEast');

hgexport(f4, 'figure_4.eps');

%% figure 5: standard deviation in bycatch rate as a function of sector bycatch rate

x = [0.002447268 0.003090404 0.006851098 0.011206277 0.013883504 0.015907889 ...
    0.017018415 0.020375476 0.020514691 0.021222183 0.024427046 0.025383037 ...
    0.025759693 0.027592404 0.028267745 0.02833822 0.029835265 0.031657561 ...
    0.032589953 0.032651153 0.037642836 0.053721259 0.057491542 0.061604504 ...
    0.075799103 0.088674059 0.134199349];
y = [0.005135358 0.0003060212 0.003536509 0.005839161 0.009499507 0.017969711 ...
    0.011869113 0.007217216 0.020260433 0.014657473 0.009911202 0.030762574 ...
    0.019561538 0.031425016 0.010898631 0.01156533 0.02085376 0.035159289 ...
    0.019682365 0.015993548 0.02308048 0.060893502 0.01708352 0.031061175 ...
    0.071078029 0.086673338 0.069535427];

y2 = 0.6855 * x;

f5 = figure(5);
set(f5, 'Position', [260 484 600 400]);
hold off;
plot(x, y2, 'Color', [0.5 0.5 0.5], 'LineWidth', 1.2);
hold on;
plot(x, y, 'd', 'Color', [0.2 0.2 0.2], 'MarkerFaceColor', [0.2 0.2 0.2]);
axis([0 0.16 0 0.1]);
xlabel('Sector bycatch rate (Chinook salmon / mt walleye pollock)', 'FontSize', 12);
ylabel('Standard Deviation in sector bycatch Rate', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;
legend('Observed', 'Modeled (y=0.6855x)', 'Location', 'SouthEast');

hgexport(f5, 'figure_5.eps');

%% figure 6: (a) bonus credits (b) penalty credits for delta 10 chinook caught
penalty_2001 = dlmread('no_trading.neg_deltas.2001.csv');
penalty_2006 = dlmread('no_trading.neg_deltas.2006.csv');
bonus_2001 = dlmread('no_trading.pos_deltas.2001.csv');
bonus_2006 = dlmread('no_trading.pos_deltas.2006.csv');

gw = 0.8;
x = 0:8;
bonus_counts_2001 = hist(bonus_2001, x);
bonus_counts_2006 = hist(bonus_2006, x);

x2 = -8:0;
penalty_counts_2001 = hist(penalty_2001, x2);
penalty_counts_2006 = hist(penalty_2006, x2);

f6 = figure(6);
set(f6, 'Position', [310 434 800 400]);

f6a = subplot(1, 2, 1);
ax = get(f6a, 'Position');
set(f6a, 'Position', ax + [-0.05, 0.02, 0.05, -0.02]);

hold off;
bar_h = bar(x-gw/4, bonus_counts_2006, gw/2);
bar_child = get(bar_h, 'Children');
hold on;
bar(x+gw/4, bonus_counts_2001, gw/2);
colormap([0.6 0.6 0.6; 0.9 0.9 0.9]);
set(bar_child,'CData',2);

axis([-0.5 8.5 0 70]);
text(-1.8, 73, 'A', 'FontSize', 13);
xlabel('ITEC bonuses', 'FontSize', 12);
ylabel('Number of vessels', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;
legend(['2006-2007, mean = ' num2str(mean(bonus_2006), 3)], ['2001-2002, mean = ' num2str(mean(bonus_2001), 3)], 'Location', 'NorthEast');

f6b = subplot(1, 2, 2);
ax = get(f6b, 'Position');
set(f6b, 'Position', ax + [0, 0.02, 0.05, -0.02]);

hold off;
bar_h = bar(x2-gw/4, penalty_counts_2006, gw/2);
bar_child = get(bar_h, 'Children');
hold on;
bar(x2+gw/4, penalty_counts_2001, gw/2);
colormap([0.6 0.6 0.6; 0.9 0.9 0.9]);
set(bar_child,'CData',2);

axis([-8.5 0.5 0 70]);
text(-9.8, 73, 'B', 'FontSize', 13);
xlabel('ITEC penalties', 'FontSize', 12);
%ylabel('Number of vessels', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;
legend(['2006-2007, mean = ' num2str(mean(penalty_2006), 3)], ['2001-2002, mean = ' num2str(mean(penalty_2001), 3)], 'Location', 'NorthWest');

hgexport(f6, 'figure_6.eps');

%% figure 7: revenue recovered with trading and reallocation
% (47,591) no reallocation, trading, no avoidance incentives
% assume $0.20/lb for A season and $0.12/lb for B season

fid = fopen('no_trading.no_realloc.47591.unfished.csv');
data = textscan(fid, '%d %d %d', 'Delimiter', ',', 'Headerlines', 1);
fclose(fid);

a_value = 0.2*2204.62262;
b_value = 0.12*2204.62262;
years = data{1};
unfished = data{2} + data{3};
orig_value = data{2} * a_value + data{3} * b_value;

fid = fopen('trading.47591.unfished.csv');
data = textscan(fid, '%d %d %d', 'Delimiter', ',', 'Headerlines', 1);
fclose(fid);

a_value = 0.2*2204.62262;
b_value = 0.12*2204.62262;
years = data{1};
unfished = data{2} + data{3};
new_value = data{2} * a_value + data{3} * b_value;

recovered = orig_value - new_value;
f7 = figure(7);
set(f7, 'Position', [360 384-50 600 450]);
bar(years, recovered, 0.6);
colormap([0.9 0.9 0.9]);
xlabel('Year', 'FontSize', 12);
ylabel('Value of recovered walleye pollock (in millions USD)', 'FontSize', 12);
set(gca, 'FontSize', 11);
axis([1999.5 2007.5 0 12000000]);
set(gca, 'YTick', 0:2000000:12000000);
set(gca, 'YtickLabel', {'0', '2', '4', '6', '8', '10', '12'});
set(gca, 'TickDir', 'out');
box off;

hgexport(f7, 'figure_7.eps');

%% figure 8: revenue recovered with trading, reallocation, and avoidance incentives
% (47,591) reallocation, trading, avoidance incentives
% assume $0.20/lb for A season and $0.12/lb for B season

fid = fopen('no_trading.no_realloc.47591.unfished.csv');
data = textscan(fid, '%d %d %d', 'Delimiter', ',', 'Headerlines', 1);
fclose(fid);

a_value = 0.2*2204.62262;
b_value = 0.12*2204.62262;
years = data{1};
unfished = data{2} + data{3};
orig_value = data{2} * a_value + data{3} * b_value;

fid = fopen('incentivized.47591.unfished.csv');
data = textscan(fid, '%d %d %d', 'Delimiter', ',', 'Headerlines', 1);
fclose(fid);

a_value = 0.2*2204.62262;
b_value = 0.12*2204.62262;
years = data{1};
unfished = data{2} + data{3};
new_value = data{2} * a_value + data{3} * b_value;

recovered = orig_value - new_value;
f8 = figure(8);
set(f8, 'Position', [410 334-50 600 450]);
bar(years, recovered, 0.6);
colormap([0.9 0.9 0.9]);
xlabel('Year', 'FontSize', 12);
ylabel('Value of recovered walleye pollock (in millions USD)', 'FontSize', 12);
set(gca, 'FontSize', 11);
axis([1999.5 2007.5 0 100000000]);
set(gca, 'YTick', 0:10000000:100000000);
set(gca, 'YtickLabel', {'0', '10', '20', '30', '40', '50', '60', '70', '80', '90', '100'});
set(gca, 'TickDir', 'out');
box off;

hgexport(f8, 'figure_8.eps');

%% figure 9: (a) total retired credits (b) yearly retired credits

actual = [1454 8868 19928 20471 31138 46355 55782 70152];
dss_held = [12483 8776 0 3258 0 0 0 0];
ftt_held = [0 91 910 735 1563 1010 437 343];

f9 = figure(9);
set(f9, 'Position', [460 284 800 400]);

f9a = subplot(1, 2, 1);
ax = get(f9a, 'Position');
set(f9a, 'Position', ax + [-0.03, 0.02, 0.05, -0.02]);

gw = 0.8;
hold off;
bar_h = bar(.5, sum(ftt_held), 0.5);
bar_child = get(bar_h, 'Children');
hold on;
bar(1.5, sum(dss_held), 0.5);
colormap([0.6 0.6 0.6; 0.9 0.9 0.9]);
set(bar_child,'CData',2);
axis([0 2 0 30000]);
text(-0.46, 31500, 'A', 'FontSize', 13);
set(gca, 'FontSize', 11);
xlabel('Trading rule', 'FontSize', 12);
ylabel('Total ITEC recovered (2000-2007)', 'FontSize', 12);
set(gca, 'XTick', [0.5 1.5]);
set(gca, 'XTickLabel', {'Fixed Trading Tax','Dynamic Salmon Savings'});
set(gca, 'YTick', 0:10000:30000);
set(gca, 'YTickLabel', {'0', '10,000', '20,000', '30,000'});
set(gca, 'TickDir', 'out');
box off;

f9b = subplot(1, 2, 2);
ax = get(f9b, 'Position');
set(f9b, 'Position', ax + [0.01, 0.02, 0.05, -0.02]);

hold off;
plot(actual, dss_held, 's', 'Color', [0.2 0.2 0.2], 'MarkerFaceColor', [0.6 0.6 0.6]);
hold on;
plot(actual, ftt_held, 'o', 'Color', [0.4 0.4 0.4], 'MarkerFaceColor', [0.9 0.9 0.9]);
xlabel('Actual yearly bycatch (Chinook salmon)', 'FontSize', 12);
ylabel('ITEC recovered', 'FontSize', 12);
set(gca, 'FontSize', 11);
axis([0 80000 0 14000]);
text(-17000, 14650, 'B', 'FontSize', 13);
set(gca, 'XTick', 0:20000:80000);
set(gca, 'XTickLabel', {'10,000', '20,000', '40,000', '60,000', '80,000'});
set(gca, 'TickDir', 'out');
box off;
legend('Dynamic Salmon Savings', 'Fixed Transfer Tax', 'Location', 'NorthEast');

hgexport(f9, 'figure_9.eps');

%% figure 10: yearly bycatch with incentives

actual = [1454 8868 19928 20471 31138 46355 55782 70152];
incentivized = [1454 7221 14383 13056 17384 22364 24319 26502];

gw = 0.8;
x = 2000:2007;

f10 = figure(10);
set(f10, 'Position', [510 234 600 400]);

hold off;
bar(x-gw/4, actual, gw/2);
hold on;
bar_h = bar(x+gw/4, incentivized, gw/2);
bar_child = get(bar_h, 'Children');
colormap([0.6 0.6 0.6; 0.9 0.9 0.9]);
set(bar_child,'CData',2);

xlabel('Year', 'FontSize', 12);
ylabel('Chinook salmon (thousands)', 'FontSize', 12);
axis([1999.5 2007.5 0 80000]);
set(gca, 'YTick', 0:20000:140000);
set(gca, 'YTickLabel', {'0', '20', '40', '60', '80', '100', '120', '140'});
set(gca, 'FontSize', 11);
set(gca, 'TickDir', 'out');
box off;
legend('Actual bycatch', 'Modeled bycatch (\psi = 0.25)', 'Location', 'NorthWest');

hgexport(f10, 'figure_10.eps');