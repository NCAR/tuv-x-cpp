#!/usr/bin/env python3
"""
Plot Delta-Eddington solver benchmark results.

Usage:
    # Generate CSV data
    ./build/test/unit/test_delta_eddington_benchmarks --csv > data/benchmarks.csv

    # Create plots
    python scripts/plot_solver_benchmarks.py data/benchmarks.csv

    # Or pipe directly
    ./build/test/unit/test_delta_eddington_benchmarks --csv | python scripts/plot_solver_benchmarks.py
"""

import sys
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

# Set style - seaborn with enhanced aesthetics
sns.set_theme(style="whitegrid", context="talk", palette="deep")
sns.set_style("whitegrid", {
    'grid.linestyle': '--',
    'grid.alpha': 0.6,
    'axes.edgecolor': '0.2',
    'axes.linewidth': 1.2,
})

# Font and figure settings
plt.rcParams.update({
    'figure.dpi': 300,
    'savefig.dpi': 300,
    'text.usetex': False,
    'mathtext.fontset': 'dejavusans',
    'font.family': 'sans-serif',
    'font.sans-serif': ['DejaVu Sans', 'Helvetica', 'Arial'],
    'font.size': 14,
    'axes.titlesize': 16,
    'axes.labelsize': 14,
    'xtick.labelsize': 12,
    'ytick.labelsize': 12,
    'legend.fontsize': 12,
    'figure.titlesize': 18,
    'axes.titleweight': 'bold',
    'axes.labelweight': 'medium',
})


def load_data(source):
    """Load CSV data from file or stdin."""
    if source == '-' or source is None:
        # Read from stdin, skip lines starting with [ (gtest output)
        lines = []
        for line in sys.stdin:
            if not line.startswith('[') and not line.startswith('Note:'):
                lines.append(line)
        from io import StringIO
        return pd.read_csv(StringIO(''.join(lines)))
    else:
        # Read from file, filtering out gtest lines
        with open(source, 'r') as f:
            lines = [line for line in f if not line.startswith('[') and not line.startswith('Note:')]
        from io import StringIO
        return pd.read_csv(StringIO(''.join(lines)))


def save_figure(fig, output_prefix, name):
    """Save figure in both PNG and PDF formats."""
    png_path = f'{output_prefix}_{name}.png'
    pdf_path = f'{output_prefix}_{name}.pdf'
    fig.savefig(png_path, dpi=300, bbox_inches='tight')
    fig.savefig(pdf_path, bbox_inches='tight')
    print(f"Saved: {png_path}, {pdf_path}")


def plot_beer_lambert_validation(df, output_prefix):
    """Plot Beer-Lambert law validation: expected vs actual transmittance."""
    # Filter Beer-Lambert tests
    bl_data = df[df['test_name'].str.contains('BeerLambert')]

    if bl_data.empty:
        print("No Beer-Lambert data found")
        return

    # Use seaborn color palette
    colors = sns.color_palette("viridis", as_cmap=True)

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Left: Expected vs Actual transmittance
    ax1 = axes[0]
    scatter = ax1.scatter(bl_data['expected_T'], bl_data['actual_T'], s=150, alpha=0.85,
                          c=bl_data['tau'], cmap='viridis', edgecolors='white', linewidth=1.5)

    # Perfect agreement line
    lims = [0, max(bl_data['expected_T'].max(), bl_data['actual_T'].max()) * 1.1]
    ax1.plot(lims, lims, '--', color='#555555', alpha=0.7, linewidth=2, label='Perfect agreement')

    ax1.set_xlabel(r'Expected Transmittance (Beer-Lambert)')
    ax1.set_ylabel(r'Actual Transmittance (Solver)')
    ax1.set_title(r'Beer-Lambert Validation')
    ax1.legend(framealpha=0.9, loc='upper left')

    # Add colorbar
    cbar = plt.colorbar(scatter, ax=ax1, pad=0.02)
    cbar.set_label(r'Optical Depth $\tau$', fontsize=13)
    cbar.ax.tick_params(labelsize=11)

    # Right: Relative error
    ax2 = axes[1]

    # Prepare data
    bl_plot = bl_data.copy()
    bl_plot['label'] = bl_plot['test_name'].str.replace('BeerLambert_', '', regex=False)
    bl_plot['error_pct'] = bl_plot['rel_error_T'] * 100

    # Check if errors are at machine precision (< 1e-12 %)
    max_error = bl_plot['error_pct'].max()
    machine_eps = 1e-13  # roughly machine precision in %

    if max_error < 1e-10:
        # Errors are at machine precision - show qualitative plot
        ax2.set_xlim(-0.5, len(bl_plot) - 0.5)
        ax2.set_ylim(0, 1)

        # Draw green checkmarks or indicators for each test
        for i, (_, row) in enumerate(bl_plot.iterrows()):
            ax2.add_patch(plt.Rectangle((i - 0.3, 0), 0.6, 0.15,
                                         facecolor=sns.color_palette("deep")[2],
                                         edgecolor='white', linewidth=2))
            ax2.text(i, 0.25, '✓', ha='center', va='bottom', fontsize=24,
                     color=sns.color_palette("deep")[2], fontweight='bold')

        ax2.set_xticks(range(len(bl_plot)))
        ax2.set_xticklabels(bl_plot['label'].tolist())
        plt.setp(ax2.get_xticklabels(), rotation=45, ha='right')

        ax2.set_xlabel('')
        ax2.set_ylabel('')
        ax2.set_title(r'Transmittance Error')

        # Add annotation
        ax2.text(0.5, 0.7, 'All errors below\nmachine precision', transform=ax2.transAxes,
                 ha='center', va='center', fontsize=14, fontweight='medium',
                 bbox=dict(boxstyle='round,pad=0.5', facecolor='#e8f5e9',
                           edgecolor=sns.color_palette("deep")[2], linewidth=2))
        ax2.text(0.5, 0.45, r'$< 10^{-13}\%$', transform=ax2.transAxes,
                 ha='center', va='center', fontsize=16, fontweight='bold',
                 color=sns.color_palette("deep")[2])

        # Hide y-axis
        ax2.set_yticks([])
        ax2.spines['left'].set_visible(False)

    else:
        # Normal case - show bar plot with log scale if needed
        sns.barplot(data=bl_plot, x='label', y='error_pct', hue='label', ax=ax2,
                    palette='viridis', edgecolor='white', linewidth=1.5, legend=False)

        ax2.set_xlabel('')
        ax2.set_ylabel(r'Relative Error (%)')
        ax2.set_title(r'Transmittance Error')

        if max_error > 0 and max_error < 0.01:
            ax2.set_yscale('log')
            ax2.axhline(y=machine_eps, color='gray', linestyle=':', linewidth=1.5,
                        alpha=0.7, label='Machine precision')

        ax2.axhline(y=0.1, color=sns.color_palette("Set1")[0], linestyle='--',
                    linewidth=2, alpha=0.7, label='0.1% tolerance')
        ax2.legend(framealpha=0.9)

        plt.setp(ax2.get_xticklabels(), rotation=45, ha='right')

        # Add value labels
        for container in ax2.containers:
            for bar in container:
                height = bar.get_height()
                if height > 1e-10:
                    ax2.annotate(f'{height:.1e}%',
                                 xy=(bar.get_x() + bar.get_width() / 2, height),
                                 ha='center', va='bottom', fontsize=9)

    plt.tight_layout(pad=2.0)
    save_figure(fig, output_prefix, 'beer_lambert')
    plt.close()


def plot_energy_conservation(df, output_prefix):
    """Plot energy conservation: R + T vs 1.0 for conservative scattering."""
    # Filter energy conservation tests
    ec_data = df[df['test_name'].str.contains('EnergyConservation')]

    if ec_data.empty:
        print("No energy conservation data found")
        return

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Calculate R + T
    ec_data = ec_data.copy()
    ec_data['R_plus_T'] = ec_data['actual_T'] + ec_data['actual_R']
    ec_data['energy_error'] = np.abs(ec_data['R_plus_T'] - 1.0) * 100

    # Color palette
    colors = sns.color_palette("deep")

    # Left: R and T breakdown
    ax1 = axes[0]
    x = np.arange(len(ec_data))
    width = 0.35

    bars1 = ax1.bar(x - width/2, ec_data['actual_T'], width,
                    label=r'Transmittance $T$', color=colors[0], edgecolor='white', linewidth=1.5)
    bars2 = ax1.bar(x + width/2, ec_data['actual_R'], width,
                    label=r'Reflectance $R$', color=colors[3], edgecolor='white', linewidth=1.5)

    # Add R+T line
    ax1.plot(x, ec_data['R_plus_T'], 'o-', markersize=10, color='#333333',
             linewidth=2.5, label=r'$R + T$', zorder=5)
    ax1.axhline(y=1.0, color=colors[2], linestyle='--', linewidth=2,
                alpha=0.7, label='Energy conserved')

    ax1.set_xticks(x)
    ax1.set_xticklabels([n.replace('EnergyConservation_', '') for n in ec_data['test_name']])
    ax1.set_ylabel(r'Fraction')
    ax1.set_title(r'Energy Balance ($\omega = 1$, conservative scattering)')
    ax1.legend(framealpha=0.9, loc='upper right')
    ax1.set_ylim(0, 1.25)

    # Add value labels on bars
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax1.annotate(f'{height:.2f}',
                         xy=(bar.get_x() + bar.get_width() / 2, height),
                         ha='center', va='bottom', fontsize=10, fontweight='medium')

    # Right: Energy error
    ax2 = axes[1]
    error_colors = [colors[2] if e < 10 else colors[1] if e < 15 else colors[3]
                    for e in ec_data['energy_error']]
    bars = ax2.bar(x, ec_data['energy_error'], color=error_colors,
                   edgecolor='white', linewidth=1.5)

    ax2.set_xticks(x)
    ax2.set_xticklabels([n.replace('EnergyConservation_', '') for n in ec_data['test_name']])
    ax2.set_ylabel(r'Energy Non-Conservation (%)')
    ax2.set_title(r'$|R + T - 1|$ (should be 0 for $\omega = 1$)')
    ax2.axhline(y=10, color=colors[1], linestyle='--', linewidth=2, alpha=0.7, label='10% tolerance')
    ax2.axhline(y=15, color=colors[3], linestyle='--', linewidth=2, alpha=0.7, label='15% tolerance')
    ax2.legend(framealpha=0.9)

    # Add value labels
    for bar in bars:
        height = bar.get_height()
        ax2.annotate(f'{height:.1f}%',
                     xy=(bar.get_x() + bar.get_width() / 2, height),
                     ha='center', va='bottom', fontsize=11, fontweight='medium')

    plt.tight_layout(pad=2.0)
    save_figure(fig, output_prefix, 'energy_conservation')
    plt.close()


def plot_parameter_space(df, output_prefix):
    """Plot solver behavior across parameter space."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))

    # Group by test category
    bl_data = df[df['test_name'].str.contains('BeerLambert')]
    ec_data = df[df['test_name'].str.contains('EnergyConservation')]

    # Color palette
    colors = sns.color_palette("deep")

    # Top-left: Transmittance vs optical depth (zenith sun only, mu0=1)
    ax1 = axes[0, 0]
    if not bl_data.empty:
        # Filter for zenith sun (mu0=1) and sort by tau
        bl_tau = bl_data[np.isclose(bl_data['mu0'], 1.0)].sort_values('tau')
        if not bl_tau.empty:
            ax1.semilogy(bl_tau['tau'], bl_tau['actual_T'], 'o-', markersize=12,
                         label='Solver', color=colors[0], linewidth=2.5,
                         markeredgecolor='white', markeredgewidth=1.5)
            tau_range = np.linspace(0.05, bl_tau['tau'].max() * 1.1, 100)
            ax1.semilogy(tau_range, np.exp(-tau_range), '--', color='#666666',
                         label=r'$e^{-\tau}$', linewidth=2, alpha=0.8)
    ax1.set_xlabel(r'Optical Depth $\tau$')
    ax1.set_ylabel(r'Transmittance $T$')
    ax1.set_title(r'Beer-Lambert: $T$ vs $\tau$ ($\mu_0 = 1$)')
    ax1.legend(framealpha=0.9)

    # Top-right: Transmittance vs mu0 (fixed tau=1)
    ax2 = axes[0, 1]
    bl_mu0 = bl_data[np.isclose(bl_data['tau'], 1.0)].sort_values('mu0') if not bl_data.empty else pd.DataFrame()
    if not bl_mu0.empty:
        ax2.plot(bl_mu0['mu0'], bl_mu0['actual_T'], 'o-', markersize=12,
                 label='Solver', color=colors[0], linewidth=2.5,
                 markeredgecolor='white', markeredgewidth=1.5)
        mu0_range = np.linspace(0.2, 1.0, 100)
        ax2.plot(mu0_range, np.exp(-1.0/mu0_range), '--', color='#666666',
                 label=r'$e^{-\tau/\mu_0}$', linewidth=2, alpha=0.8)
    ax2.set_xlabel(r'$\mu_0 = \cos(\theta)$')
    ax2.set_ylabel(r'Transmittance $T$')
    ax2.set_title(r'Beer-Lambert: $T$ vs $\mu_0$ ($\tau = 1$)')
    ax2.legend(framealpha=0.9)

    # Bottom-left: R vs T for scattering cases
    ax3 = axes[1, 0]
    if not ec_data.empty:
        scatter = ax3.scatter(ec_data['actual_T'], ec_data['actual_R'],
                              c=ec_data['g'], cmap='coolwarm', s=200,
                              edgecolors='white', linewidth=2, zorder=5)
        # R + T = 1 line
        ax3.plot([0, 1], [1, 0], '--', color='#666666', linewidth=2,
                 alpha=0.8, label=r'$R + T = 1$')
        cbar = plt.colorbar(scatter, ax=ax3, pad=0.02)
        cbar.set_label(r'Asymmetry factor $g$', fontsize=13)
        cbar.ax.tick_params(labelsize=11)
    ax3.set_xlabel(r'Transmittance $T$')
    ax3.set_ylabel(r'Reflectance $R$')
    ax3.set_title(r'Scattering: $R$ vs $T$ ($\omega = 1$)')
    ax3.legend(framealpha=0.9)
    ax3.set_xlim(0, 1.1)
    ax3.set_ylim(0, 0.6)

    # Bottom-right: Summary statistics
    ax4 = axes[1, 1]
    ax4.axis('off')

    # Calculate statistics
    stats_lines = []
    stats_lines.append("Benchmark Summary")
    stats_lines.append("=" * 35)
    stats_lines.append("")

    if not bl_data.empty:
        bl_max_err = bl_data['rel_error_T'].max() * 100
        stats_lines.append("Beer-Lambert Tests")
        stats_lines.append("-" * 35)
        stats_lines.append(f"  Number of tests:     {len(bl_data)}")
        stats_lines.append(f"  Max relative error:  {bl_max_err:.2e} %")
        stats_lines.append(f"  Status:              {'PASS' if bl_max_err < 0.1 else 'FAIL'}")
        stats_lines.append("")

    if not ec_data.empty:
        ec_data_copy = ec_data.copy()
        ec_data_copy['R_plus_T'] = ec_data_copy['actual_T'] + ec_data_copy['actual_R']
        ec_max_err = (ec_data_copy['R_plus_T'] - 1.0).abs().max() * 100
        stats_lines.append("Energy Conservation Tests")
        stats_lines.append("-" * 35)
        stats_lines.append(f"  Number of tests:     {len(ec_data)}")
        stats_lines.append(f"  Max |R+T-1|:         {ec_max_err:.2f} %")
        stats_lines.append(f"  Status:              {'PASS' if ec_max_err < 15 else 'FAIL'}")

    stats_text = "\n".join(stats_lines)

    ax4.text(0.05, 0.95, stats_text, transform=ax4.transAxes, fontsize=13,
             verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round,pad=0.8', facecolor='#f0f0f0',
                       edgecolor='#cccccc', linewidth=1.5))

    plt.tight_layout(pad=2.5)
    save_figure(fig, output_prefix, 'parameter_space')
    plt.close()


def main():
    parser = argparse.ArgumentParser(description='Plot Delta-Eddington benchmark results')
    parser.add_argument('input', nargs='?', default='-',
                        help='Input CSV file (default: stdin)')
    parser.add_argument('-o', '--output', default='benchmark',
                        help='Output prefix for plots (default: benchmark)')
    args = parser.parse_args()

    print(f"Loading data from {'stdin' if args.input == '-' else args.input}...")
    df = load_data(args.input)
    print(f"Loaded {len(df)} test results")
    print(f"Tests: {', '.join(df['test_name'].unique())}")

    # Create output directory if needed
    import os
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Generate plots
    print("\nGenerating plots...")
    plot_beer_lambert_validation(df, args.output)
    plot_energy_conservation(df, args.output)
    plot_parameter_space(df, args.output)

    print("\nDone!")


if __name__ == '__main__':
    main()
