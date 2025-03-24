#!/usr/bin/env python3
"""
Plotting Utility for Model Builder Tool

This script generates various plots for regression models and other ML visualizations.
It provides a unified interface for all plotting functionality in the application.

Usage:
    Called from the C++ application with appropriate arguments.
    
Arguments:
    --plot_type: Type of plot to generate (scatter, timeseries, residual, importance, learning_curve, nn_architecture, tree)
    --data_file: Path to CSV file containing the data
    --output_file: Path to save the plot
    --title: Plot title
    
    Additional arguments depending on plot type:
    For scatter:
        --x_column: Name of the X column
        --y_column: Name of the Y column
        --model_file: Path to file with predictions
        
    For importance:
        --features_json: JSON string of feature names and importance scores
        
    For learning_curve:
        --width: Width of the plot in inches
        --height: Height of the plot in inches
        
    For nn_architecture:
        --layer_sizes: Comma-separated list of integers representing layer sizes
        
    For tree:
        --tree_structure: JSON string representing the tree structure
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os
import sys
import json
from pathlib import Path
import matplotlib

# Set the backend to Agg for non-interactive environments
matplotlib.use('Agg')

def create_scatter_plot(data, model_data, x_column, y_column, title, output_file, width=10, height=6):
    """Create scatter plot of actual vs predicted values."""
    plt.figure(figsize=(width, height))
    
    # Get actual and predicted values
    actual = data[y_column]
    predicted = model_data['predicted']
    
    # Create scatter plot
    plt.scatter(actual, predicted, alpha=0.6, label='Data Points')
    
    # Add perfect prediction line
    min_val = min(actual.min(), predicted.min())
    max_val = max(actual.max(), predicted.max())
    plt.plot([min_val, max_val], [min_val, max_val], 'r--', label='Perfect Prediction')
    
    # Add labels and title
    plt.xlabel(x_column)
    plt.ylabel(y_column)
    plt.title(title)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150, bbox_inches='tight')
    plt.close()

def create_timeseries_plot(data, title, output_file, width=12, height=6):
    """Create time series plot of actual and predicted values."""
    plt.figure(figsize=(width, height))
    
    # Check if the data contains both actual and predicted values
    has_actual = 'actual' in data.columns
    has_predicted = 'predicted' in data.columns
    
    # Create time series plot
    if has_actual:
        plt.plot(data.index, data['actual'], label='Actual', alpha=0.7, linewidth=1)
    if has_predicted:
        plt.plot(data.index, data['predicted'], label='Predicted', alpha=0.7, linewidth=1)
    
    # Add labels and title
    plt.xlabel('Time Index', fontsize=8)
    plt.ylabel('Values', fontsize=8)
    plt.title(title, fontsize=10)
    plt.xticks(fontsize=8)
    plt.yticks(fontsize=8)
    plt.legend(fontsize=8)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150, bbox_inches='tight')
    plt.close()

def create_importance_plot(data, title, output_file, width=10, height=6):
    """Create bar plot of feature importance scores."""
    plt.figure(figsize=(width, height))
    
    # Sort data by importance if not already sorted
    if 'importance' in data.columns and 'feature' in data.columns:
        data = data.sort_values('importance', ascending=True)
        
        # Create horizontal bar plot
        y_pos = np.arange(len(data['feature']))
        plt.barh(y_pos, data['importance'], align='center', height=0.5)
        plt.yticks(y_pos, data['feature'], fontsize=8)
    else:
        # Handle the case where data is passed as a JSON string
        try:
            features_json = data.iloc[0, 0] if not data.empty else "{}"
            features = json.loads(features_json)
            names = list(features.keys())
            scores = list(features.values())
            
            # Sort by importance
            sorted_idx = np.argsort(scores)
            pos = np.arange(len(sorted_idx)) + .5
            
            # Create horizontal bar plot
            plt.barh(pos, np.array(scores)[sorted_idx], align='center')
            plt.yticks(pos, np.array(names)[sorted_idx])
        except:
            print("Error parsing feature importance data", file=sys.stderr)
    
    # Add labels and title
    plt.xlabel('Relative Importance', fontsize=8)
    plt.title(title, fontsize=10)
    plt.xticks(fontsize=8)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.tight_layout(pad=0.5)
    plt.savefig(output_file, format='png', dpi=150, bbox_inches='tight')
    plt.close()

def create_residual_plot(data, title, output_file, width=10, height=6):
    """Create residual plot of predicted vs residual values."""
    plt.figure(figsize=(width, height))
    
    # Ensure the data has the required columns
    if 'predicted' in data.columns and 'residual' in data.columns:
        # Create residual plot
        plt.scatter(data['predicted'], data['residual'], alpha=0.6)
        plt.axhline(y=0, color='r', linestyle='-')
    else:
        # If 'residual' not provided, try to calculate it from 'actual' and 'predicted'
        if 'actual' in data.columns and 'predicted' in data.columns:
            data['residual'] = data['actual'] - data['predicted']
            plt.scatter(data['predicted'], data['residual'], alpha=0.6)
            plt.axhline(y=0, color='r', linestyle='-')
        else:
            print("Error: Required columns not found for residual plot", file=sys.stderr)
    
    # Add labels and title
    plt.xlabel('Predicted Values')
    plt.ylabel('Residuals')
    plt.title(title)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def create_learning_curve_plot(data, title, output_file, width=10, height=6):
    """Create learning curve plot showing training and validation scores vs training sizes."""
    plt.figure(figsize=(width, height))
    
    # Create learning curve plot
    if all(col in data.columns for col in ['training_size', 'training_score', 'validation_score']):
        plt.plot(data['training_size'], data['training_score'], 'o-', label='Training score')
        plt.plot(data['training_size'], data['validation_score'], 'o-', label='Validation score')
        
        # Add labels and title
        plt.xlabel('Training Set Size')
        plt.ylabel('Score')
        plt.title(title)
        plt.legend(loc='best')
        plt.grid(True, linestyle='--', alpha=0.7)
    else:
        print("Error: Required columns not found for learning curve plot", file=sys.stderr)
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def draw_neural_net(ax, left, right, bottom, top, layer_sizes):
    """Draw a neural network diagram on the given axis."""
    n_layers = len(layer_sizes)
    v_spacing = (top - bottom) / float(max(layer_sizes))
    h_spacing = (right - left) / float(n_layers - 1)
    
    # Nodes
    for n, layer_size in enumerate(layer_sizes):
        layer_top = v_spacing * (layer_size - 1) / 2. + (top + bottom) / 2.
        for m in range(layer_size):
            circle = plt.Circle((n * h_spacing + left, layer_top - m * v_spacing), v_spacing / 4.,
                              color='w', ec='k', zorder=4)
            ax.add_artist(circle)
            
    # Edges
    for n, (layer_size_a, layer_size_b) in enumerate(zip(layer_sizes[:-1], layer_sizes[1:])):
        layer_top_a = v_spacing * (layer_size_a - 1) / 2. + (top + bottom) / 2.
        layer_top_b = v_spacing * (layer_size_b - 1) / 2. + (top + bottom) / 2.
        for m in range(layer_size_a):
            for o in range(layer_size_b):
                line = plt.Line2D([n * h_spacing + left, (n + 1) * h_spacing + left],
                                 [layer_top_a - m * v_spacing, layer_top_b - o * v_spacing], c='k')
                ax.add_artist(line)

def create_neural_network_architecture_plot(layer_sizes, title, output_file, width=12, height=8):
    """Create a neural network architecture visualization."""
    fig = plt.figure(figsize=(width, height))
    ax = fig.gca()
    ax.axis('off')
    
    # Convert layer_sizes to list of integers if it's a string
    if isinstance(layer_sizes, str):
        try:
            layer_sizes = [int(x) for x in layer_sizes.split(',')]
        except ValueError:
            print("Error: Invalid layer sizes format", file=sys.stderr)
            layer_sizes = [4, 6, 5, 3]  # Default if parsing fails
    
    # Draw the neural network
    draw_neural_net(ax, 0, 1, 0, 1, layer_sizes)
    
    # Add title
    plt.title(title)
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def create_tree_visualization_plot(tree_structure, title, output_file, width=12, height=8):
    """Create a decision tree visualization."""
    # This is a simplified version - in a real implementation, you would parse the tree structure
    # and use libraries like networkx to visualize it properly
    
    plt.figure(figsize=(width, height))
    plt.text(0.5, 0.5, "Tree Visualization\n(Placeholder - Implement full tree viz)",
             horizontalalignment='center', verticalalignment='center', fontsize=14)
    plt.title(title)
    plt.axis('off')
    
    # Save plot
    plt.tight_layout()
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def main():
    parser = argparse.ArgumentParser(description='Generate various plots for Model Builder Tool')
    
    # Required arguments
    parser.add_argument('--plot_type', required=True, 
                        choices=['scatter', 'timeseries', 'residual', 'importance', 
                                'learning_curve', 'nn_architecture', 'tree'],
                        help='Type of plot to generate')
    parser.add_argument('--data_file', required=True, 
                        help='Path to CSV file containing the data')
    parser.add_argument('--output_file', required=True, 
                        help='Path to save the plot')
    parser.add_argument('--title', default='Model Results', 
                        help='Plot title')
    
    # Optional arguments depending on plot type
    parser.add_argument('--model_file', 
                        help='Path to CSV file containing model predictions')
    parser.add_argument('--x_column', 
                        help='Name of the X column')
    parser.add_argument('--y_column', 
                        help='Name of the Y column')
    parser.add_argument('--features_json', 
                        help='JSON string of feature names and importance scores')
    parser.add_argument('--width', type=float, default=10.0, 
                        help='Width of the plot in inches')
    parser.add_argument('--height', type=float, default=6.0, 
                        help='Height of the plot in inches')
    parser.add_argument('--layer_sizes', 
                        help='Comma-separated list of integers representing layer sizes')
    parser.add_argument('--tree_structure', 
                        help='JSON string representing the tree structure')
    
    args = parser.parse_args()
    
    try:
        # Convert paths to absolute paths using Path
        data_file = Path(args.data_file).resolve()
        output_file = Path(args.output_file).resolve()
        
        # Create output directory if it doesn't exist
        os.makedirs(output_file.parent, exist_ok=True)
        
        # Load data
        data = pd.read_csv(data_file)
        
        # Process based on plot type
        if args.plot_type == 'scatter':
            if not args.model_file or not args.x_column or not args.y_column:
                raise ValueError("Scatter plots require model_file, x_column, and y_column arguments")
            
            model_file = Path(args.model_file).resolve()
            model_data = pd.read_csv(model_file)
            
            create_scatter_plot(data, model_data, args.x_column, args.y_column, 
                              args.title, str(output_file), args.width, args.height)
        
        elif args.plot_type == 'timeseries':
            create_timeseries_plot(data, args.title, str(output_file), args.width, args.height)
        
        elif args.plot_type == 'residual':
            create_residual_plot(data, args.title, str(output_file), args.width, args.height)
        
        elif args.plot_type == 'importance':
            if args.features_json:
                # If features are passed as JSON string
                with open(data_file, 'w') as f:
                    f.write(args.features_json)
                
            create_importance_plot(data, args.title, str(output_file), args.width, args.height)
        
        elif args.plot_type == 'learning_curve':
            create_learning_curve_plot(data, args.title, str(output_file), args.width, args.height)
        
        elif args.plot_type == 'nn_architecture':
            if not args.layer_sizes:
                raise ValueError("Neural network architecture plots require layer_sizes argument")
            
            create_neural_network_architecture_plot(args.layer_sizes, args.title, str(output_file), 
                                                 args.width, args.height)
        
        elif args.plot_type == 'tree':
            if not args.tree_structure:
                raise ValueError("Tree visualization plots require tree_structure argument")
            
            create_tree_visualization_plot(args.tree_structure, args.title, str(output_file), 
                                         args.width, args.height)
        
        print(f"Plot saved as: {output_file}")
        sys.exit(0)
        
    except Exception as e:
        print(f"Error creating plot: {str(e)}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()