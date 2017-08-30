%% figure a-1: linear bycatch function

fa1 = figure(11);
set(fa1, 'Position', [610 684 600 400]);

x = -3:0.2:3;
y = 5/3 * ones(size(x));
k = find(x > -2);
y(k) = 1 - 1/3 * x(k);
k = find(x > 2);
y(k) = repmat(1/3, size(k));
plot(x, y, '-', 'Color', [0.2, 0.2, 0.2], 'LineWidth', 1.2);
xlabel('\it{z}\rm{-score}', 'FontSize', 12);
ylabel('\it{Q}', 'FontSize', 12);
set(gca, 'FontSize', 11);
grid on;
axis([-3 3, 0 2]);
set(gca, 'YTick', 0:1/3:2);
set(gca, 'YTickLabel', {'0', '1/3', '2/3', '1', '4/3', '5/3', '2'});
set(gca, 'TickDir', 'out');
box off;

hgexport(fa1, 'figure_a1.eps');

%% figure a-2: normal cdf bycatch function

fa2 = figure(12);
set(fa2, 'Position', [660 634 600 400]);

x = -3:0.01:3;
y = 5/3 - 4/3 * normcdf(x, 0, 1);

plot(x, y, '-', 'Color', [0.2, 0.2, 0.2], 'LineWidth', 1.2);
xlabel('\it{z}\rm{-score}', 'FontSize', 12);
ylabel('\it{Q}', 'FontSize', 12);
set(gca, 'FontSize', 11);
grid on;
axis([-3 3, 0 2]);
set(gca, 'YTick', 0:1/3:2);
set(gca, 'YTickLabel', {'0', '1/3', '2/3', '1', '4/3', '5/3', '2'});
set(gca, 'TickDir', 'out');
box off;

hgexport(fa2, 'figure_a2.eps');

%% figure a-3: convergence rates

fa3 = figure(13);
set(fa3, 'Position', [710 584 800 400]);

num_years = 5;

x = 1:num_years;
y1 = ones(size(x));
y2 = ones(size(x));


fa3a = subplot(1, 2, 1);
ax = get(fa3a, 'Position');
set(fa3a, 'Position', ax + [-0.05, 0.02, 0.05, -0.02]);
for i = 1:(num_years-1)
    y1(i+1) = 1/3 + 1/3 * y1(i) + 1/3 * 1/3;
    y2(i+1) = 1/4 + 1/2 * y2(i) + 1/4 * 1/3;
end
hold off;
plot(x, y1, '-s', 'Color', [0.2, 0.2, 0.2], 'LineWidth', 1.2, 'MarkerSize', 6, 'MarkerFaceColor', [0.2, 0.2, 0.2]);
hold on;
plot(x, y2, '-o', 'Color', [0.4, 0.4, 0.4], 'LineWidth', 1.2, 'MarkerSize', 6, 'MarkerFaceColor', [0.4, 0.4, 0.4]);

axis([1 num_years 0.5 1.5]);
text(0.4, 1.55, 'A', 'FontSize', 13);
grid on;
xlabel('Year', 'FontSize', 12)
ylabel('Proportional Allocation Factor', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'YTick', 0.5:1/6:1.5);
set(gca, 'YTickLabel', {'1/2', '2/3', '5/6', '1', '7/6', '4/3', '3/2'});
legend('1/3 1/3 1/3', '1/4 1/2 1/4', 'Location', 'NorthEast');

fa3b = subplot(1, 2, 2);
ax = get(fa3b, 'Position');
set(fa3b, 'Position', ax + [0, 0.02, 0.05, -0.02]);
for i = 1:(num_years-1)
    y1(i+1) = 1/3 + 1/3 * y1(i) + 1/3 * 5/3;
    y2(i+1) = 1/4 + 1/2 * y2(i) + 1/4 * 5/3;
end
hold off;
plot(x, y1, '-s', 'Color', [0.2, 0.2, 0.2], 'LineWidth', 1.2, 'MarkerSize', 6, 'MarkerFaceColor', [0.2, 0.2, 0.2]);
hold on;
plot(x, y2, '-o', 'Color', [0.4, 0.4, 0.4], 'LineWidth', 1.2, 'MarkerSize', 6, 'MarkerFaceColor', [0.4, 0.4, 0.4]);

axis([1 num_years 0.5 1.5]);
text(0.4, 1.55, 'B', 'FontSize', 13);
grid on;
xlabel('Year', 'FontSize', 12)
% ylabel('Proportional Allocation Factor', 'FontSize', 12);
set(gca, 'FontSize', 11);
set(gca, 'YTick', 0.5:1/6:1.5);
set(gca, 'YTickLabel', {'1/2', '2/3', '5/6', '1', '7/6', '4/3', '3/2'});
legend('1/3 1/3 1/3', '1/4 1/2 1/4', 'Location', 'NorthWest');
set(gca, 'TickDir', 'out');
box off;

hgexport(fa3, 'figure_a3.eps');

%% figure b-1: total sector bycatch in B season

years = 2000:2007;
vals = -ones(366, 8);
x = -ones(366, 8);
for i = 1:8
    filename = ['no_trading.no_realloc.47591.' num2str(years(i)) '.csv'];
    fid = fopen(filename);
    data = textscan(fid, '%s %*d %*f %f %*d %d %*f', 'Delimiter', ',', 'Headerlines', 1);
    fclose(fid);
    dates = datenum(datevec(data{1}, 'mmm-dd-yyyy')) - datenum(years(i),1,1) + 1;
    pollock_std = data{2};
    bycatch_std = data{3};
    
    if(mod(years(i), 4) == 0)
        feb_29 = find(dates == 60);
        bycatch_std(feb_29:end-1) = bycatch_std(feb_29+1:end);
        pollock_std(feb_29:end-1) = pollock_std(feb_29+1:end);
    end
    vals(dates,i) = bycatch_std;
    x(dates, i) = pollock_std;
end

pollock = x(162:end,:);
bycatch = vals(162:end,:);

styles = {'.', 'd', 's', '+', '*', 'o', '^', 'v'};

fb1 = figure(14);
set(fb1, 'Position', [760 534 600 400]);

hold off;
for(i = 2:8)
    k = find(pollock(:,i) > 0 & bycatch(:,i) > 0);
    last = max(k);
    k = k(1:2:end);
    plot(pollock(k,i)/pollock(last,i), bycatch(k,i)/bycatch(last,i), styles{i}, 'Color', [0.3 0.3 0.3]);
    hold on;
end

axis([0 1.0 0 1.0]);
set(gca, 'YTick', 0:0.2:1);
xlabel('Fraction of total sector pollock caught', 'FontSize', 12);
ylabel('Fraction of total sector bycatch caught', 'FontSize', 12);
set(gca, 'FontSize', 11);
legend('2001', '2002', '2003', '2004', '2005', '2006', '2007', 'Location', 'NorthWest');
set(gca, 'TickDir', 'out');
box off;

hgexport(fb1, 'figure_b1.eps');